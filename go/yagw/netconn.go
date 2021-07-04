package main

import (
	"log"
	"sync/atomic"
	"syscall"
)

type netconn struct {
	pd       polldesp
	bad      int32
	auth     int32
	busy     int32
	wpending int32
	pr       *packet_reader
	ravail   chan struct{}
	wavail   chan struct{}
	down     chan struct{}
	g_start  bool
}

func (c *netconn) fd() int {
	return c.pd.fd
}

func (c *netconn) Close() error {
	if !atomic.CompareAndSwapInt32(&c.bad, 0, 1) {
		return nil
	}
	free_pd(&c.pd)
	close(c.down)
	return syscall.Close(c.fd())
}

func (c *netconn) on_poll() (reading bool, writing bool) {
	// on_poll 要求尽量不能有让出时间片副作用的操作
	// 典型副作用来自于内存分配和 chan 在多线程的竞争
	if atomic.LoadInt32(&c.bad) != 0 {
		return
	}

	// busy 状态相当于 pr 的信号量并且维护了 pr 的临界区
	// 如果没有消费这个 busy 信号，就不会读取新请求
	// busy 信号一定来自于读消息完成
	var pr *packet_reader
	if atomic.LoadInt32(&c.busy) == 0 {
		pr = c.pr
	}
	for pr != nil && c.pd.readable() {
		atomic.StoreInt32(&c.pd.r, 0)
		ready, err := pr.readfrom(c.fd())
		if err != nil {
			log.Printf("read request: %v", err)
			c.Close()
			return
		}
		reading = !ready
		if ready {
			atomic.StoreInt32(&c.pd.r, 1)
			atomic.StoreInt32(&c.busy, 1)
			select {
			case c.ravail <- struct{}{}:
			default:
			}
			break
		}
	}

	writing = atomic.LoadInt32(&c.wpending) != 0
	if writing && c.pd.writable() {
		atomic.StoreInt32(&c.pd.w, 0)
		atomic.StoreInt32(&c.wpending, 0)
		select {
		case c.wavail <- struct{}{}:
		default:
		}
		writing = false
	}

	if !c.g_start && atomic.LoadInt32(&c.busy) != 0 {
		go c.mainproc()
		c.g_start = true
	}
	return
}

func (c *netconn) flush_wbuf(buf []byte) (int, syscall.Errno) {
	p := 0
	for p < len(buf) {
		n, en := sys_write(c.fd(), buf[p:])
		if en != 0 {
			if en == syscall.EAGAIN {
				atomic.StoreInt32(&c.wpending, 1)
				if c.pd.writable() {
					// 这里不是 CAS 可能会和 on_poll 产生重复事件，但不影响正确性
					atomic.StoreInt32(&c.pd.w, 0)
					atomic.StoreInt32(&c.wpending, 0)
					continue
				}
				break
			}
			return p, en
		}
		p += n
	}
	return p, 0
}

func (c *netconn) mainproc() {
	ctx := new_client(c)
	select {
	case <-c.ravail:
		// 要求第一个请求必须是登录请求
		ok, uid, err := ctx.handle_login(c.pr)
		if err != nil {
			log.Printf("handle login: %v", err)
			c.Close()
			return
		}
		c.pr.reset()
		if ok {
			atomic.StoreInt32(&c.auth, 1)
			ctx.auth_init(uid)
		}
	case <-c.down:
		return
	}
	for {
		// 因为所有 chan 都是非阻塞发送的，所以可以使用 cond 来代替 chan 的多路复用
		select {
		case msg := <-ctx.deliver:
			if len(ctx.wbuf)+len(msg) <= WBUF_SIZE_LIMIT {
				ctx.wbuf = append(ctx.wbuf, msg...)
			}
		case <-c.ravail:
			err := ctx.handle(c.pr)
			if err != nil {
				log.Printf("handle: %v", err)
				c.Close()
				return
			}
			c.pr.reset()
		case <-c.wavail:
		case <-c.down:
			atomic.StoreInt32(&ctx.bad, 1)
			return
		}
		n, en := c.flush_wbuf(ctx.wbuf)
		if en != 0 {
			log.Printf("flush wbuf: %v", en)
			c.Close()
			return
		}
		if n > 0 {
			m := copy(ctx.wbuf, ctx.wbuf[n:])
			ctx.wbuf = ctx.wbuf[:m]
		}
		// 如果写缓冲堆积太多数据也要禁止读请求
		if atomic.LoadInt32(&c.busy) != 0 && len(ctx.wbuf) < WBUF_SIZE_LIMIT {
			atomic.StoreInt32(&c.busy, 0)
			// on_poll 处理读事件时可能处于 busy，导致解除 busy 状态时错过读事件
			// 目前如果读可用直接加入到全局队列中，后期考虑优化
			if c.pd.readable() {
				// 这里不考虑 select c.down
				server.gq <- &c.pd
			}
		}
	}
}

func new_netconn(fd int) *netconn {
	c := &netconn{
		pr:       &packet_reader{},
		down:     make(chan struct{}),
		ravail:   make(chan struct{}, 1),
		wavail:   make(chan struct{}, 1),
		wpending: 1, // 接收第一个写事件信号
	}
	c.pd.fd = fd
	c.pd.c = c
	return c
}
