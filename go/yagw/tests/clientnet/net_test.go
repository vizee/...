package clientnet

import (
	"encoding/json"
	"fmt"
	"log"
	"testing"
	"time"
)

func TestConnection(t *testing.T) {
	c, err := Login("127.0.0.1:9249", "123", "token")
	if err != nil {
		t.Fatalf("login: %v", err)
	}
	defer c.Close()
	go func() {
		for {
			cmd, seq, data, err := ReadPacket(c)
			if err != nil {
				t.Fatalf("read ping: %v", err)
			}
			if cmd != CMD_PING {
				t.Fatalf("unexpected cmd: %d", cmd)
			}
			var tm struct {
				Ts int64 `json:"ts"`
			}
			json.Unmarshal(data, &tm)
			log.Printf("ping %d => %d\n", seq, time.Now().UnixNano()-tm.Ts)
		}
	}()
	for i := 0; i < 5; i++ {
		ts := time.Now().UnixNano()
		err := WritePacket(c, CMD_PING, nseq, []byte(fmt.Sprintf(`{"ts":%d}`, ts)))
		if err != nil {
			t.Fatalf("write ping: %v", err)
		}
		time.Sleep(time.Second * 5)
	}
}
