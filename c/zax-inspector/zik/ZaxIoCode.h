#ifndef ZAX_IOCODE_H
#define ZAX_IOCODE_H

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x00000022
#endif
#ifndef METHOD_NEITHER
#define METHOD_NEITHER 3
#endif
#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS 0
#endif
#ifndef CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#endif

#define IOCTL_BASE (0x800)

#define IOCTL_CODE(_func) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE + (_func), METHOD_NEITHER, FILE_ANY_ACCESS)

//ZIK io control code
#define ZIK_IO_DRV_INIT		IOCTL_CODE(0x02)
#define ZIK_IO_DRV_UNLOAD	IOCTL_CODE(0x03)
#define ZIK_IO_SSDT_R		IOCTL_CODE(0x04)
#define ZIK_IO_SSDT_W		IOCTL_CODE(0x05)
#define ZIK_IO_SSDTS_R		IOCTL_CODE(0x06)
#define ZIK_IO_SSDTS_W		IOCTL_CODE(0x07)
#define ZIK_IO_IDT_R		IOCTL_CODE(0x08)
#define ZIK_IO_KERNEL_R		IOCTL_CODE(0x09)
#define ZIK_IO_KERNEL_W		IOCTL_CODE(0x0a)

#endif
