// +build amd64

package sys

func lockOr32(addr *uint32, val uint32)
func lockCmpxchg8(addr *byte, cmp byte, val byte) (ok bool)
