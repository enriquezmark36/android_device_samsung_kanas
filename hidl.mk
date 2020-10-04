# Camera
PRODUCT_PACKAGES += \
	android.hardware.camera.provider@2.4-service \

# Configstore
# Used by some services like SurfaceFlinger to store
# its configuration.
PRODUCT_PACKAGES += \
	android.hardware.configstore@1.1-service \

# Graphics
# NOTE: SurfaceFlinger will start the allocator service via the
# SF_START_GRAPHICS_ALLOCATOR_SERVICE := true
# flag on BoardConfig.mk
# So this might not be needed anymore:
#	android.hardware.graphics.allocator@2.0-service
PRODUCT_PACKAGES += \
	android.hardware.graphics.allocator@2.0-impl \
	android.hardware.graphics.composer@2.1-impl \

# Lights
PRODUCT_PACKAGES += \
	android.hardware.light@2.0-service \

# Media
PRODUCT_PACKAGES += \
	android.hardware.cas@1.0-service \

# Memtrack
PRODUCT_PACKAGES += \
	android.hardware.memtrack@1.0-impl \

# Sensors
PRODUCT_PACKAGES += \
	android.hardware.sensors@1.0-service \

#Power
PRODUCT_PACKAGES := $(filter-out android.hardware.power@1.0-service.sc8830,$(PRODUCT_PACKAGES))
PRODUCT_PACKAGES += \
	android.hardware.power@1.0-service.kanas

#Usb
# Remove the default usb service added by scx35-common before adding ours
PRODUCT_PACKAGES := $(filter-out android.hardware.usb@1.0-service,$(PRODUCT_PACKAGES))
PRODUCT_PACKAGES += android.hardware.usb@1.0-service.kanas
