package main

import (
	"log"
	"net"
	"runtime"
	"strconv"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
	"unsafe"
)

const RECYCLEPD_LIMIT = 512

const (
	POLL_R  = 1
	POLL_W  = 2
	POLL_RW = POLL_R | POLL_W
)

type polldesp struct {
	bad int32
	own int32
	r   int32
	w   int32
	fd  int
	c   *netconn

	local_next *polldesp
	free_next  *polldesp
}

func (pd *polldesp) readable() bool {
	return atomic.LoadInt32(&pd.r) != 0
}

func (pd *polldesp) writable() bool {
	return atomic.LoadInt32(&pd.w) != 0
}

type freelist struct {
	mu  sync.Mutex
	pd0 *polldesp
	num int
	pd1 *polldesp
}

func (l *freelist) push(pd *polldesp) bool {
	toomany := false
	l.mu.Lock()
	pd.free_next = l.pd0
	l.pd0 = pd
	num := l.num
	if num < RECYCLEPD_LIMIT {
		l.num++
		switch num {
		case 0:
			l.pd1 = pd
		case RECYCLEPD_LIMIT - 1:
			toomany = true
		}
	}
	l.mu.Unlock()
	return toomany
}

func (l *freelist) clear() {
	l.mu.Lock()
	l.pd0 = nil
	l.num = 0
	l.pd1 = nil
	l.mu.Unlock()
}

var server struct {
	epfd int

	event_fd int
	event_pd *polldesp

	listen_fd int
	listen_pd *polldesp

	gq chan *polldesp

	freepds   freelist
	recycling int32
}

func install_pd(fd int, rw int, pd *polldesp) {
	events := syscall.EPOLLET | syscall.EPOLLRDHUP
	if rw&POLL_R != 0 {
		events |= syscall.EPOLLIN
	}
	if rw&POLL_W != 0 {
		events |= syscall.EPOLLOUT
	}
	ev := syscall.EpollEvent{
		Events: uint32(events),
	}
	*(**polldesp)(unsafe.Pointer(&ev.Fd)) = pd
	err := syscall.EpollCtl(server.epfd, syscall.EPOLL_CTL_ADD, fd, &ev)
	if err != nil {
		log.Fatalf("EpollCtl(ADD): %v", err)
	}
}

func raise_recyclepd() {
	if atomic.CompareAndSwapInt32(&server.recycling, 0, 1) {
		dummy := [8]byte{1}
		_, err := sys_write(server.event_fd, dummy[:])
		if err != 0 {
			log.Panicf("sys_write(): %v", err)
		}
	}
}

func free_pd(pd *polldesp) {
	if !atomic.CompareAndSwapInt32(&pd.bad, 0, 1) {
		return
	}
	// 直到 file description 的最后一个 file descriptor 被 close 才会自动从 epoll 中移除
	// 所以需要手动 epoll del，避免已 close 的 fd 反复触发 epoll
	err := syscall.EpollCtl(server.epfd, syscall.EPOLL_CTL_DEL, pd.fd, nil)
	if err != nil {
		// NOTE 手动 epoll del 情况下，别的线程先调用了 close，可能会有错误
		log.Printf("EpollCtl(DEL): %v", err)
	}
	recycle := server.freepds.push(pd)
	if recycle && atomic.LoadInt32(&server.recycling) == 0 {
		raise_recyclepd()
	}
}

func listen_tcp(addr string) int {
	na, err := net.ResolveTCPAddr("tcp", addr)
	if err != nil {
		log.Fatalf("resolve address %s: %v", addr, err)
	}
	ip := na.IP
	if ip == nil {
		ip = net.IPv6zero
	} else {
		ip = ip.To4()
		if ip == nil {
			ip = na.IP
		}
	}
	var (
		af int
		sa syscall.Sockaddr
	)
	if len(ip) == net.IPv4len {
		sa4 := &syscall.SockaddrInet4{Port: na.Port}
		copy(sa4.Addr[:], ip)
		sa = sa4
		af = syscall.AF_INET
	} else if len(ip) == net.IPv6len {
		sa6 := &syscall.SockaddrInet6{Port: na.Port}
		copy(sa6.Addr[:], ip.To16())
		nif, err := net.InterfaceByName(na.Zone)
		if err == nil {
			sa6.ZoneId = uint32(nif.Index)
		} else {
			n, _ := strconv.Atoi(na.Zone)
			sa6.ZoneId = uint32(n)
		}
		sa = sa6
		af = syscall.AF_INET6
	}
	s, err := syscall.Socket(af, syscall.SOCK_STREAM|syscall.SOCK_NONBLOCK, 0)
	if err != nil {
		log.Fatalf("Socket(): %v", err)
	}
	err = syscall.Bind(s, sa)
	if err != nil {
		log.Fatalf("Bind(): %v", err)
	}
	err = syscall.Listen(s, syscall.SOMAXCONN)
	if err != nil {
		log.Fatalf("Listen(): %v", err)
	}
	return s
}

func accept_proc(acceptable <-chan struct{}) {
	for range acceptable {
		for {
			fd, err := sys_accept4(server.listen_fd, 0, 0, syscall.SOCK_NONBLOCK)
			if err != 0 {
				if err == syscall.EAGAIN {
					break
				}
				log.Fatalf("sys_accept4(): %v", err)
			}

			log.Printf("accept connection: %d", fd)
			c := new_netconn(fd)
			install_pd(fd, POLL_RW, &c.pd)
			set_timeout((*nonameconn)(c), LOGIN_TIMEOUT)
		}
	}
}

func worker_proc(wq <-chan *polldesp) {
	var (
		localq0 *polldesp
		localq1 *polldesp
	)
	for {
		var pd *polldesp
		select {
		case pd = <-wq:
			if !atomic.CompareAndSwapInt32(&pd.own, 0, 1) {
				continue
			}
		default:
		}
		if pd == nil && localq1 != nil {
			pd = localq1
			localq1 = pd.local_next
			if localq1 == nil {
				localq0 = nil
			}
			pd.local_next = nil
		}
		if pd == nil {
			// 阻塞收取的情况下会考虑从 gq 收取
			select {
			case pd = <-wq:
			case pd = <-server.gq:
			}
			if !atomic.CompareAndSwapInt32(&pd.own, 0, 1) {
				continue
			}
		}
		if atomic.LoadInt32(&pd.bad) != 0 {
			continue
		}
		reading, writing := pd.c.on_poll()
		atomic.StoreInt32(&pd.own, 0)
		// 原则上只要保证 netconn 不要饥饿，不需要立即处理下一个请求
		// 在 ET 模式下，如果 pd 的 r 和 w 可用，那么可能 epoll 接下来不会触发 pd
		// netconn 不处于 reading 时说明有请求还没被处理
		// 如果处理请求后有回复，那么可以通过下一个 w 可用事件让 pd 进入全局队列
		// 如果没有回复，在处理请求后，需要检查 r 和 own，并将 pd 重新加入本地队列
		if (pd.readable() && reading) || (pd.writable() && writing) {
			if atomic.CompareAndSwapInt32(&pd.own, 0, 1) {
				if localq0 != nil {
					localq0.local_next = pd
					localq0 = pd
				} else {
					localq0 = pd
					localq1 = pd
				}
			}
		}
	}
}

func server_eventloop() {
	acceptable := make(chan struct{}, 1)
	go accept_proc(acceptable)

	nevents := runtime.NumCPU() * 4
	if nevents < 64 {
		nevents = 64
	}
	nworkers := runtime.NumCPU() * 4
	maxevents := nworkers * 2
	if maxevents < nevents {
		maxevents = nevents
	}
	wq := make([]chan *polldesp, nworkers)
	for i := 0; i < nworkers; i++ {
		wq[i] = make(chan *polldesp, nevents)
		go worker_proc(wq[i])
	}
	server.gq = make(chan *polldesp, maxevents)

	events := make([]syscall.EpollEvent, nevents)
	for {
		n, err := syscall.EpollWait(server.epfd, events, -1)
		if err != nil {
			if no, ok := err.(syscall.Errno); ok && no == syscall.EINTR {
				continue
			}
			log.Fatalf("EpollWait(): %v", err)
		}
		for i := 0; i < n; i++ {
			pd := *(**polldesp)(unsafe.Pointer(&events[i].Fd))
			if atomic.LoadInt32(&pd.bad) != 0 {
				continue
			}
			switch pd.fd {
			case server.event_fd:
				var dummy [8]byte
				_, en := sys_read(server.event_fd, dummy[:])
				if en != 0 {
					log.Fatalf("sys_read(): %v", en)
				}
			case server.listen_fd:
				select {
				case acceptable <- struct{}{}:
				default:
				}
			default:
				events := events[i].Events
				if events&(syscall.EPOLLIN|syscall.EPOLLERR|syscall.EPOLLHUP) != 0 {
					atomic.StoreInt32(&pd.r, 1)
				}
				if events&(syscall.EPOLLOUT|syscall.EPOLLERR|syscall.EPOLLRDHUP|syscall.EPOLLHUP) != 0 {
					atomic.StoreInt32(&pd.w, 1)
				}
				if atomic.LoadInt32(&pd.own) == 0 {
					select {
					case wq[pd.fd%nworkers] <- pd:
					default:
						server.gq <- pd
					}
				}
			}
		}

		server.freepds.clear()
		atomic.StoreInt32(&server.recycling, 0)
	}
}

func tick_recyclepd() {
	for range time.Tick(time.Minute * 5) {
		raise_recyclepd()
	}
}

func server_start(addr string) {
	var err error
	server.epfd, err = syscall.EpollCreate1(0)
	if err != nil {
		log.Fatalf("EpollCreate1(): %v", err)
	}

	var en syscall.Errno
	server.event_fd, en = sys_eventfd2(0, EFD_NONBLOCK)
	if en != 0 {
		log.Fatalf("sys_eventfd2(): %v", en)
	}
	server.event_pd = &polldesp{
		fd: server.event_fd,
	}
	install_pd(server.event_fd, POLL_R, server.event_pd)

	server.listen_fd = listen_tcp(addr)
	server.listen_pd = &polldesp{
		fd: server.listen_fd,
	}
	install_pd(server.listen_fd, POLL_R, server.listen_pd)

	go tick_recyclepd()
}
