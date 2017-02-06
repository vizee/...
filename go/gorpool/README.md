# gorpool
goroutine pool

## install

```text
go get github.com/vizee/gorpool
```

## usage

```go
package main

import (
    "time"
    "unsafe"

    "github.com/vizee/asm/hack"
    "github.com/vizee/gorpool"
)

func getstack() {
    tls := hack.TLS()
    println("tls", tls)
    lo := *(*uintptr)(unsafe.Pointer(tls))
    println("lo", lo)
    hi := *(*uintptr)(unsafe.Pointer(tls + 8))
    println("hi", hi)
    println("size", hi-lo)
}

func main() {
    pool := &gorpool.Pool{Reserve: 4096}
    pool.Go(func() {
        getstack()
    })
    pool.Go(func() {
        getstack()
    })
    time.Sleep(time.Second)
    pool.Go(func() {
        getstack()
    })
    time.Sleep(time.Second)
}
```
