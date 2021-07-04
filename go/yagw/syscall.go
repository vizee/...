package main

import (
	"syscall"
	"unsafe"
)

const (
	EFD_NONBLOCK = 04000
)

func sys_read(fd int, p []byte) (int, syscall.Errno) {
	n, _, err := syscall.RawSyscall(syscall.SYS_READ, uintptr(fd), *(*uintptr)(unsafe.Pointer(&p)), uintptr(len(p)))
	return int(n), err
}

func sys_write(fd int, p []byte) (int, syscall.Errno) {
	n, _, err := syscall.RawSyscall(syscall.SYS_WRITE, uintptr(fd), *(*uintptr)(unsafe.Pointer(&p)), uintptr(len(p)))
	return int(n), err
}

func sys_accept4(fd int, addr uintptr, addrlen uintptr, flags int) (int, syscall.Errno) {
	r, _, err := syscall.RawSyscall6(syscall.SYS_ACCEPT4, uintptr(fd), addr, addrlen, uintptr(flags), 0, 0)
	return int(r), err
}

func sys_eventfd2(initval uint, flags int) (int, syscall.Errno) {
	r, _, err := syscall.RawSyscall(syscall.SYS_EVENTFD2, uintptr(initval), uintptr(flags), 0)
	return int(r), err
}
