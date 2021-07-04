package main

import (
	"runtime"
	"sync/atomic"
	"time"

	"github.com/vizee/timer"
)

var (
	timeridx uint64
	timers   []timer.Timer
)

func set_timeout(h timer.Handler, after time.Duration) {
	idx := atomic.AddUint64(&timeridx, 1) % uint64(len(timers))
	timers[idx].Add(h, time.Now().UnixNano()+int64(after))
}

func init() {
	timers = make([]timer.Timer, runtime.NumCPU())
}
