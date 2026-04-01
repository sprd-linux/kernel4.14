PRODUCT_PACKAGES += chipone-tddi.ko
PRODUCT_COPY_FILES += vendor/sprd/modules/devdrv/input/touchscreen/chipone/init.chipone-tddi.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.chipone-tddi.rc \
			vendor/sprd/modules/devdrv/input/touchscreen/chipone/chipone-tddi.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/chipone-tddi.kl
