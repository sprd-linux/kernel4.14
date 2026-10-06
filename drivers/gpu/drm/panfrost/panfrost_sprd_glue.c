// SPDX-License-Identifier: GPL-2.0
/* SPRD sharkle glue for panfrost: GPU power domain bring-up.
 * The stock sprd,mali-midgard driver cleared the PMU force-shutdown bit
 * via the "syscons" property; without it the GPU registers read 0 and the
 * soft reset times out (-110). */
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/delay.h>
#include "panfrost_device.h"

int panfrost_syscons_enable(struct panfrost_device *pfdev)
{
	struct device_node *np = pfdev->dev->of_node;
	struct device_node *sysnp;
	struct regmap *map;
	u32 ph, reg, mask;
	int i, ret;

	for (i = 0; ; i++) {
		if (of_property_read_u32_index(np, "syscons", i * 3, &ph))
			break;
		ret = of_property_read_u32_index(np, "syscons", i * 3 + 1, &reg);
		ret |= of_property_read_u32_index(np, "syscons", i * 3 + 2, &mask);
		if (ret)
			break;
		sysnp = of_find_node_by_phandle(ph);
		if (!sysnp)
			return -EINVAL;
		map = syscon_node_to_regmap(sysnp);
		of_node_put(sysnp);
		if (IS_ERR(map))
			return PTR_ERR(map);
		regmap_update_bits(map, reg, mask, 0);
		dev_info(pfdev->dev, "syscons[%d]: write reg=%#x mask=%#x\n", i, reg, mask);
	}
	if (i)
		msleep(50);
	dev_info(pfdev->dev, "gpu power domain on (%d syscons entries)\n", i);
	return i ? 0 : -ENODATA;
}
