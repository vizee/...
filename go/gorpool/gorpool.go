package gorpool

import (
	"sync"
	"time"
)

type gor struct {
	ch  chan func()
	idx int
}

type Pool struct {
	mu      sync.Mutex
	idle    []*gor
	Reserve uint
}

func reserve(kb uint) {
	var stack [1024]byte
	_ = stack[1023]
	if kb <= 1 {
		return
	}
	reserve(kb - 1)
}

func (p *Pool) gorproc(g *gor) {
	// only a new gor reserves stack
	if r := p.Reserve; r >= 1024 {
		reserve(r / 1024)
	}
	tm := time.NewTimer(time.Minute)
	down := false
	for !down {
		tm.Reset(time.Minute)
		select {
		case fn := <-g.ch:
			fn()
			p.mu.Lock()
			g.idx = len(p.idle)
			p.idle = append(p.idle, g)
			p.mu.Unlock()
		case <-tm.C:
			p.mu.Lock()
			if g.idx >= 0 {
				t := p.idle[len(p.idle)-1]
				t.idx = g.idx
				p.idle[t.idx] = t
				p.idle = p.idle[:len(p.idle)-1]
				down = true
			}
			p.mu.Unlock()
		}
	}
}

func (p *Pool) Go(fn func()) {
	if fn == nil {
		panic("nil func")
	}
	var g *gor
	p.mu.Lock()
	if len(p.idle) > 0 {
		g = p.idle[len(p.idle)-1]
		p.idle = p.idle[:len(p.idle)-1]
		g.idx = -1
	}
	p.mu.Unlock()
	if g == nil {
		g = &gor{
			ch:  make(chan func(), 1),
			idx: -1,
		}
		go p.gorproc(g)
	}
	g.ch <- fn
}
