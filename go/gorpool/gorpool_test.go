package gorpool

import (
	"fmt"
	"runtime"
	"sync"
	"testing"
)

func TestPool(t *testing.T) {
	p := Pool{}
	wg := sync.WaitGroup{}
	for i := 0; i < 1000; i++ {
		wg.Add(1)
		p.Go(func() {
			fmt.Println("do", runtime.NumGoroutine())
			wg.Done()
		})
	}
	wg.Wait()
}
