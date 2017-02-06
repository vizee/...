package sys

import (
	"bytes"
	"sync"
	"sync/atomic"
	"unsafe"
)

type PollFunc func(*File, uint32)

type polldesp struct {
	f    *File
	pf   PollFunc
	slot uintptr
	idx  int
}

func (pd *polldesp) recycle() {
	pd.f = nil
	pd.pf = nil
}

const slotSize = 128

type pdslot struct {
	marks [slotSize]byte
	pds   [slotSize]polldesp
	pnext uintptr // *pdslot
	cnt   int32
}

func (s *pdslot) init() {
	for i := 0; i < slotSize; i++ {
		s.pds[i].slot = uintptr(unsafe.Pointer(s))
		s.pds[i].idx = i
	}
}

type pdpool struct {
	m       sync.Mutex
	slots   []*pdslot
	partial uintptr // *pdslot
}

func (p *pdpool) putpartial(slot *pdslot) {
	// TODO solve ABA problem
	for {
		slot.pnext = atomic.LoadUintptr(&p.partial)
		if atomic.CompareAndSwapUintptr(&p.partial, slot.pnext, uintptr(unsafe.Pointer(slot))) {
			break
		}
		//		runtime.Gosched()
	}
}

func (p *pdpool) alloc() *polldesp {
	for {
		slot := (*pdslot)(unsafe.Pointer(atomic.LoadUintptr(&p.partial)))
		if slot == nil {
			break
		}
		cnt := atomic.AddInt32(&slot.cnt, 1)
		if cnt > slotSize {
			atomic.AddInt32(&slot.cnt, -1)
			atomic.CompareAndSwapUintptr(&p.partial, uintptr(unsafe.Pointer(slot)), slot.pnext)
			//		runtime.Gosched()
			continue
		}
		i := 0
		for {
			i = bytes.IndexByte(slot.marks[:], 0)
			if i != -1 && lockCmpxchg8(&slot.marks[i], 0, 1) {
				break
			}
			i++
		}
		return &slot.pds[i]
	}
	slot := new(pdslot)
	slot.init()
	slot.cnt = 1
	slot.marks[0] = 1
	p.m.Lock()
	p.slots = append(p.slots, slot)
	p.m.Unlock()
	p.putpartial(slot)
	return &slot.pds[0]
}

func (p *pdpool) free(pd *polldesp) {
	pd.recycle()
	slot := *(**pdslot)(unsafe.Pointer(&pd.slot))
	slot.marks[pd.idx] = 0
	if atomic.AddInt32(&slot.cnt, -1) == slotSize-1 {
		p.putpartial(slot)
	}
}
