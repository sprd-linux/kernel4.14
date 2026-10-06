/* SPDX-License-Identifier: GPL-2.0
 * 5.10 -> 4.14 backport shims for panfrost / drm scheduler / gem shmem.
 * Auto-included via ccflags -include for these objects only. */
#ifndef _PANFROST_4_14_SHIM_H
#define _PANFROST_4_14_SHIM_H
#include <linux/reservation.h>
#include <linux/sched.h>
#include <drm/drmP.h>
#include <drm/drm_gem.h>

/* dma_resv_* (5.3+) -> reservation_object_* (4.14) */
#define dma_resv_get_excl_rcu		reservation_object_get_excl_rcu
#define dma_resv_add_excl_fence		reservation_object_add_excl_fence
#define dma_resv_add_shared_fence	reservation_object_add_shared_fence
#define dma_resv_wait_timeout_rcu	reservation_object_wait_timeout_rcu
#define dma_resv_init			reservation_object_init
#define dma_resv_fini			reservation_object_fini
#define dma_resv_test_signaled_rcu	reservation_object_test_signaled_rcu
#define dma_resv_list			reservation_object_list
#define dma_resv			reservation_object

/* vm_fault_t (4.17+): 4.14 的 fault 处理已用 VM_FAULT_* 位标志，仅类型名不同 */
#include <linux/mm.h>
typedef int vm_fault_t;
/* 5.10 vmf_insert_page(): 成功时返回 VM_FAULT_NOPAGE（PTE 已由驱动装好），
 * 失败返回 VM_FAULT_* 位标志；4.14 vm_insert_page() 成功返回 0、失败返回 errno。
 * 若直接透传，4.14 __do_fault 会把 0 当成"已设置 vmf->page"而 lock_page(NULL)
 * →内核 oops（mesa panfrost 触碰 mmap 后必现）。
 */
static inline vm_fault_t vmf_insert_page(struct vm_area_struct *vma,
					 unsigned long addr, struct page *page)
{
	int err = vm_insert_page(vma, addr, page);

	switch (err) {
	case 0:
		return VM_FAULT_NOPAGE;
	case -ENOMEM:
		return VM_FAULT_OOM;
	default:
		return VM_FAULT_SIGBUS;
	}
}
/* drm_printf_indent (5.x): 4.14 的 drm_printf 无 indent */
#define drm_printf_indent(p, indent, fmt, ...)	drm_printf(p, fmt, ##__VA_ARGS__)

#ifndef DRM_DEBUG_PRIME
#define DRM_DEBUG_PRIME(fmt, ...)	do { } while (0)
#endif

/* sgtable 迭代器/映射 (5.x) */
#include <linux/dma-mapping.h>
#define for_each_sgtable_dma_sg(sgt, sg, i) \
	for_each_sg((sgt)->sgl, sg, (sgt)->nents, i)
#define for_each_sgtable_page(sgt, pg, i) \
	for_each_page_in_sgtable? /* unused */
#define dma_map_sgtable(dev, sgt, dir, attrs) \
	((dma_map_sg((dev), (sgt)->sgl, (sgt)->nents, (dir)) == (sgt)->nents) ? 0 : -EIO)
#include <linux/sizes.h>
#ifndef SZ_4G
#define SZ_4G			0x100000000ULL
#endif
/* 4.14 devfreq governor 名常量在私有 governor.h */
#ifndef DEVFREQ_GOV_SIMPLE_ONDEMAND
#define DEVFREQ_GOV_SIMPLE_ONDEMAND	"simple_ondemand"
#endif

/* drm_gem_(un)lock_reservations: use the 4.14 ww-mutex API. */
#include <linux/ww_mutex.h>
static inline bool panfrost_reservation_is_duplicate(struct drm_gem_object **objs,
						     int index)
{
	int i;

	for (i = 0; i < index; i++)
		if (objs[i]->resv == objs[index]->resv)
			return true;
	return false;
}

static inline int drm_gem_lock_reservations(struct drm_gem_object **objs,
					    int count,
					    struct ww_acquire_ctx *acquire_ctx)
{
	int contended = -1;
	int i, ret;

	ww_acquire_init(acquire_ctx, &reservation_ww_class);

retry:
	if (contended != -1) {
		ret = ww_mutex_lock_slow_interruptible(&objs[contended]->resv->lock,
						       acquire_ctx);
		if (ret)
			goto fail;
	}

	for (i = 0; i < count; i++) {
		if (i == contended || panfrost_reservation_is_duplicate(objs, i))
			continue;

		ret = ww_mutex_lock_interruptible(&objs[i]->resv->lock, acquire_ctx);
		if (ret) {
			int j;

			for (j = 0; j < i; j++)
				if (!panfrost_reservation_is_duplicate(objs, j))
					reservation_object_unlock(objs[j]->resv);
			if (contended >= i)
				reservation_object_unlock(objs[contended]->resv);
			if (ret == -EDEADLK) {
				contended = i;
				goto retry;
			}
			goto fail;
		}
	}

	ww_acquire_done(acquire_ctx);
	return 0;

fail:
	ww_acquire_done(acquire_ctx);
	ww_acquire_fini(acquire_ctx);
	return ret;
}
static inline void drm_gem_unlock_reservations(struct drm_gem_object **objs,
					       int count,
					       struct ww_acquire_ctx *acquire_ctx)
{
	int i;

	for (i = 0; i < count; i++)
		if (!panfrost_reservation_is_duplicate(objs, i))
			reservation_object_unlock(objs[i]->resv);
	ww_acquire_fini(acquire_ctx);
}

/* dma_fence_begin/end_signalling (5.x lockdep 注解) */
#define dma_fence_begin_signalling()		false
#define dma_fence_end_signalling(x)		do { (void)(x); } while (0)

/* devm_clk_get_optional (4.16+) */
#include <linux/clk.h>
static inline struct clk *devm_clk_get_optional(struct device *dev, const char *id)
{
	struct clk *c = devm_clk_get(dev, id);
	if (IS_ERR(c))
		return NULL;
	return c;
}
/* dev_pm_domain_attach_by_name (5.x): sharkle DT has none -> NULL */
#include <linux/pm_domain.h>
#define dev_pm_domain_attach_by_name(dev, name)	NULL
/* dev_pm_domain_detach (4.14) */

/* dma_unmap_sgtable (5.x) */
#define dma_unmap_sgtable(dev, sgt, dir, attrs) \
	dma_unmap_sg((dev), (sgt)->sgl, (sgt)->nents, (dir))

/* 4.14 GEM put requires struct_mutex; use its unlocked variant.
 * GEM get is already a plain kref_get and needs no compatibility alias. */
#define drm_gem_object_put		drm_gem_object_put_unlocked

/* drm_dev_put (4.15+) */
#define drm_dev_put			drm_dev_unref
/* drm_gem_prime_mmap (4.15+): 4.14 的 drm_gem_mmap 对自有 gem 对象等价 */
#define drm_gem_prime_mmap		drm_gem_mmap
/* drm_timeout_abs_to_jiffies (5.x) */
static inline unsigned long drm_timeout_abs_to_jiffies(int64_t timeout)
{
	ktime_t deadline, now;
	u64 jiffies_timeout;

	if (timeout == 0)
		return 0;
	deadline = ns_to_ktime(timeout);
	now = ktime_get();
	if (!ktime_after(deadline, now))
		return 0;
	jiffies_timeout = nsecs_to_jiffies64(ktime_to_ns(ktime_sub(deadline, now)));
	if (jiffies_timeout >= MAX_SCHEDULE_TIMEOUT - 1)
		return MAX_SCHEDULE_TIMEOUT - 1;
	return jiffies_timeout + 1;
}

/* sched_set_fifo_low() (5.10+) */
#define sched_set_fifo_low(p) do { \
	struct sched_param __sp = { .sched_priority = 1 }; \
	sched_setscheduler(p, SCHED_FIFO, &__sp); \
} while (0)
#endif
