package main

type document struct {
	A struct {
		A int         `json:"a"`
		B bool        `json:"b"`
		C string      `json:"c"`
		D interface{} `json:"d"`
		E []string    `json:"e"`
		F []struct {
			A int    `json:"a"`
			B string `json:"b"`
		} `json:"f"`
	} `json:"a"`
}
