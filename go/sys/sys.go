package sys

import (
	"sync"
)

var onceInit sync.Once

func Init() (err error) {
	onceInit.Do(func() {
		err = pollInit()
	})
	return
}

func PollWait(ignoreIntr bool) error {
	return pollWait(ignoreIntr)
}
