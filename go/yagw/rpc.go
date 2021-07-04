package main

import (
	"strconv"
	"time"
)

func rpc_auth_user(user string, token string) (bool, int32, error) {
	time.Sleep(50 * time.Millisecond)
	uid, err := strconv.Atoi(user)
	return true, int32(uid), err
}
