package main

import (
	"os"
	"path/filepath"
)

func main() {
	if len(os.Args) != 2 {
		println("usage: json2go json")
		return
	}
	jsonfile, err := filepath.Abs(os.Args[1])
	if err != nil {
		println(err.Error())
		return
	}
	fjson, err := os.Open(jsonfile)
	if err != nil {
		println(err.Error())
		return
	}
	defer fjson.Close()
	fgo, err := os.Create(jsonfile + ".go")
	if err != nil {
		println(err.Error())
		return
	}
	defer fgo.Close()
	err = json2go(fjson, fgo, "main", "document")
	if err != nil {
		println(err.Error())
	}
}
