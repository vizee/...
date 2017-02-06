package main

import (
	"encoding/json"
	"io/ioutil"
	"os"
	"testing"
)

func TestJson2Go(t *testing.T) {
	fjson, err := os.Open("test.json")
	if err != nil {
		t.Error(err.Error())
		return
	}
	defer fjson.Close()
	fgo, err := os.Create("test.json.go")
	if err != nil {
		t.Error(err.Error())
		return
	}
	defer fgo.Close()
	err = json2go(fjson, fgo, "main", "document")
	if err != nil {
		t.Error(err.Error())
	}
}

func TestGo2Json(t *testing.T) {
	data, err := ioutil.ReadFile("test.json")
	if err != nil {
		t.Error(err.Error())
		return
	}
	var doc document
	err = json.Unmarshal(data, &doc)
	if err != nil {
		t.Error(err.Error())
	}
	t.Log(doc)
}
