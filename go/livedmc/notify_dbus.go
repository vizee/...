// +build linux

package main

import "github.com/godbus/dbus"

type notifier struct {
	conn    *dbus.Conn
	appName string
	delay   int
}

func (n *notifier) init() error {
	conn, err := dbus.SessionBus()
	if err != nil {
		return err
	}
	n.conn = conn
	return nil
}

func (n *notifier) show(summary string, body string) error {
	obj := n.conn.Object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
	var (
		actions []string
		hints   map[string]dbus.Variant
	)
	call := obj.Call("org.freedesktop.Notifications.Notify", 0, n.appName, uint32(0), "", summary, body, actions, hints, int32(n.delay))
	return call.Err
}

func (n *notifier) close() {
	n.conn.Close()
}
