package main

import (
	"encoding/binary"
	"encoding/json"
	"encoding/xml"
	"flag"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

const (
	header_ober      uint16 = 0x0001
	header_msg       uint16 = 0x0004
	header_subscribe uint16 = 0x0101
	header_heartbeat uint16 = 0x0102
)

type (
	proto_ober struct {
		Ober uint32 // 观众数
	}

	proto_msg struct {
		Size uint16 // 包长度
		// entity string // 一个长度为 Size - 4 的 json 字符串
	}

	proto_subscribe struct {
		Header   uint16 // 01 01
		U16_000c uint16 // 00 0c
		Roomid   uint32 // 房间号
		Uid      uint32 // 用户id 没登陆就是 0
	}

	proto_heartbeat struct {
		Header   uint16 // 01 02
		U16_0004 uint16 // 00 04
	}
)

var (
	optVerbose   = false
	optDebug     = false
	optNotify    = false
	optHeartBeat = 0
	isXterm      = strings.Contains(os.Getenv("TERM"), "xterm")
)

func fatal(s string) {
	fmt.Println(s)
	panic(s)
}

func verbose(s string) {
	if optVerbose {
		fmt.Println(s)
	}
}

func println(s string) {
	fmt.Println(s)
}

func printmsg(uts int64, uname string, msg string) {
	ts := time.Unix(uts, 0).Format("15:04:05")
	if isXterm {
		fmt.Printf("\033[32m%s \033[36m<%s>\033[0m %s\n", ts, uname, msg)
	} else {
		fmt.Printf("%s <%s> %s\n", ts, uname, msg)
	}
}

type client struct {
	state  int
	conn   net.Conn
	tick   *time.Ticker
	notify notifier
}

func (c *client) connect(roomid int, uid int) error {
	resp, err := http.Get("http://live.bilibili.com/api/player?id=cid:" + strconv.Itoa(roomid))
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	decoder := xml.NewDecoder(resp.Body)
	found := false
	server := ""
	for {
		t, err := decoder.Token()
		if err != nil {
			if err == io.EOF {
				break
			}
			return err
		}
		switch node := t.(type) {
		case xml.StartElement:
			if node.Name.Local == "server" {
				found = true
			}
		case xml.EndElement:
			if node.Name.Local == "server" {
				found = false
			}
		case xml.CharData:
			if found {
				server = string(node)
			}
		}
		if server != "" {
			break
		}
	}
	verbose("remote server: " + server)
	c.conn, err = net.Dial("tcp", server+":88")
	if err != nil {
		return err
	}
	defer func() {
		if c.state == 0 {
			c.conn.Close()
		}
	}()
	verbose("remote: " + c.conn.RemoteAddr().String())
	println("danmuku server connected")
	subscribe := proto_subscribe{
		Header:   header_subscribe,
		U16_000c: 0x000c,
		Roomid:   uint32(roomid),
		Uid:      uint32(uid),
	}
	err = binary.Write(c.conn, binary.BigEndian, &subscribe)
	if err != nil {
		return err
	}
	c.state = 1
	return nil
}

func (c *client) heartbeat() {
	retry := 0
	hbdata := [...]byte{0x01, 0x02, 0x00, 0x04}
	c.tick = time.NewTicker(time.Duration(optHeartBeat) * time.Second)
	for range c.tick.C {
		if _, err := c.conn.Write(hbdata[:]); err != nil {
			retry++
			if retry > 3 {
				println("heartbeat failed")
				return
			}
		} else {
			verbose("heartbeat: " + time.Now().Format("15:04:05"))
			retry = 0
		}
	}
}

func (c *client) mainloop() error {
	var (
		header   uint16
		oberData proto_ober
		msgData  proto_msg
		buf      = make([]byte, 256)
	)
	for c.state == 1 {
		err := binary.Read(c.conn, binary.BigEndian, &header)
		if err != nil {
			return err
		}
		switch header {
		case header_ober:
			err = binary.Read(c.conn, binary.BigEndian, &oberData)
			if err != nil {
				return err
			}
			verbose("ober: " + strconv.Itoa(int(oberData.Ober)))
		case header_msg:
			err = binary.Read(c.conn, binary.BigEndian, &msgData)
			if err != nil {
				return err
			}
			n := int(msgData.Size) - 4
			if n > cap(buf) {
				buf = make([]byte, n)
			} else {
				buf = buf[:n]
			}
			if _, err := io.ReadFull(c.conn, buf); err != nil {
				return err
			} else {
				if optDebug {
					fmt.Println("client.mainloop.buf", string(buf))
				}
				var entity map[string]interface{}
				err := json.Unmarshal(buf, &entity)
				if err != nil {
					verbose(string(buf))
					return err
				}
				cmd := entity["cmd"].(string)
				if cmd == "DANMU_MSG" {
					info := entity["info"].([]interface{})
					data := info[0].([]interface{})
					uts := int64(data[4].(float64))
					user := info[2].([]interface{})
					uname := user[1].(string)
					msg := info[1].(string)
					printmsg(uts, uname, msg)
					if optNotify {
						c.notify.show("danmuku", fmt.Sprintf("[%s] %s", uname, msg))
					}
				} else if cmd == "SEND_GIFT" {
					data := entity["data"].(map[string]interface{})
					giftName := data["giftName"].(string)
					num := int(data["num"].(float64))
					uname := data["uname"].(string)
					uts := int64(data["timestamp"].(float64))
					msg := giftName + " x " + strconv.Itoa(num)
					printmsg(uts, uname, msg)
					if optNotify {
						c.notify.show("gift", fmt.Sprintf("[%s] %s", uname, msg))
					}
				} else {
					verbose("unknown: " + cmd)
				}
			}
		}
	}
	return nil
}

func (c *client) close() {
	if c.conn != nil {
		c.conn.Close()
	}
	if c.tick != nil {
		c.tick.Stop()
	}
	c.notify.close()
	c.state = 0
}

func main() {
	flag.BoolVar(&optVerbose, "v", false, "show verbose")
	flag.BoolVar(&optDebug, "d", false, "show debug info")
	flag.BoolVar(&optNotify, "n", false, "display notification")
	flag.IntVar(&optHeartBeat, "h", 25, "heartbeat interval")
	flag.Parse()
	if !flag.Parsed() || flag.NArg() != 1 {
		fatal("livedmc [-v] roomid")
	}
	if optVerbose {
		optDebug = true
	}
	roomid, err := strconv.Atoi(flag.Args()[0])
	if err != nil {
		fatal(err.Error())
	}
	c := client{}
	err = c.connect(roomid, 0)
	if err != nil {
		fatal(err.Error())
	}
	err = c.notify.init()
	if err != nil {
		fatal(err.Error())
	}
	c.notify.appName = "livedmc"
	c.notify.delay = 3000
	defer c.close()
	go c.heartbeat()
	err = c.mainloop()
	if err != nil {
		fatal(err.Error())
	}
}
