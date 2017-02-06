// +build !linux

package main

type notifier struct {
	appName string
	delay   int
}

func (n *notifier) init() error {
	return nil
}

func (n *notifier) show(summary string, body string) error {
	return nil
}

func (n *notifier) close() {
}
