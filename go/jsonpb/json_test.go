package jsonpb

import (
	"encoding/json"
	"testing"
)

var jsondata = []byte(`[
  {
    "_id": "5e097bc74f53847a57f297d8",
    "index": 0,
    "guid": "e5f5899d-2c89-4f43-b863-59108d38fc37",
    "isActive": true,
    "balance": "$2,242.41",
    "picture": "http://placehold.it/32x32",
    "age": 26,
    "eyeColor": "brown",
    "name": "Mavis Davis",
    "gender": "female",
    "company": "SEQUITUR",
    "email": "mavisdavis@sequitur.com",
    "phone": "+1 (992) 600-3306",
    "address": "765 Herzl Street, Shaft, Alaska, 6135",
    "about": "Consequat ullamco eu aliqua aliquip Lorem eiusmod adipisicing eu magna ex eiusmod aliqua. Do aute proident non cupidatat cupidatat sit cupidatat. Exercitation id do adipisicing mollit duis officia reprehenderit fugiat esse do aliquip id. Ipsum minim velit nulla esse aute do ea aute ad Lorem ea dolor.\r\n",
    "registered": "2018-04-24T01:41:01 -08:00",
    "latitude": 18.844651,
    "longitude": 141.206022,
    "tags": [
      "Lorem",
      "ad",
      "eu",
      "elit",
      "non",
      "adipisicing",
      "tempor"
    ],
    "friends": [
      {
        "id": 0,
        "name": "Rosa Maxwell"
      },
      {
        "id": 1,
        "name": "Gay Lowe"
      },
      {
        "id": 2,
        "name": "Dyer Frank"
      }
    ],
    "greeting": "Hello, Mavis Davis! You have 1 unread messages.",
    "favoriteFruit": "apple"
  },
  {
    "_id": "5e097bc75633e26742c5d3af",
    "index": 1,
    "guid": "c5108de9-5d03-4d44-85ed-f333423e0306",
    "isActive": true,
    "balance": "$2,241.11",
    "picture": "http://placehold.it/32x32",
    "age": 35,
    "eyeColor": "blue",
    "name": "Blake Warren",
    "gender": "male",
    "company": "MAGNAFONE",
    "email": "blakewarren@magnafone.com",
    "phone": "+1 (980) 479-2527",
    "address": "913 Menahan Street, Darrtown, Hawaii, 9381",
    "about": "Tempor aliquip qui veniam aute duis eiusmod reprehenderit id eu consequat reprehenderit dolore do sunt. Sit dolore culpa irure incididunt laboris cillum dolore commodo irure dolore culpa. Consequat qui adipisicing amet cupidatat consectetur minim ut duis velit magna et non sint aliquip. Anim ea consectetur elit voluptate laborum et dolor est elit proident veniam laborum veniam.\r\n",
    "registered": "2016-12-27T05:21:15 -08:00",
    "latitude": 34.976203,
    "longitude": -65.983947,
    "tags": [
      "sint",
      "qui",
      "elit",
      "enim",
      "voluptate",
      "occaecat",
      "consectetur"
    ],
    "friends": [
      {
        "id": 0,
        "name": "Beulah Henry"
      },
      {
        "id": 1,
        "name": "Marian Dillon"
      },
      {
        "id": 2,
        "name": "Snyder Gill"
      }
    ],
    "greeting": "Hello, Blake Warren! You have 4 unread messages.",
    "favoriteFruit": "banana"
  },
  {
    "_id": "5e097bc778b1648979eb647e",
    "index": 2,
    "guid": "a70db040-e737-4b9e-8b1c-7f49b282d82f",
    "isActive": false,
    "balance": "$3,999.44",
    "picture": "http://placehold.it/32x32",
    "age": 28,
    "eyeColor": "green",
    "name": "Le Guy",
    "gender": "male",
    "company": "DEVILTOE",
    "email": "leguy@deviltoe.com",
    "phone": "+1 (898) 431-2074",
    "address": "640 Manhattan Court, Norwood, Colorado, 3642",
    "about": "Ad incididunt nostrud reprehenderit pariatur nulla labore adipisicing sunt. Minim et mollit incididunt dolor consequat quis ut fugiat. Sint sit velit officia qui consectetur veniam reprehenderit aliquip ea qui nulla.\r\n",
    "registered": "2018-05-20T06:52:53 -08:00",
    "latitude": 78.999192,
    "longitude": -101.425375,
    "tags": [
      "aliquip",
      "labore",
      "qui",
      "fugiat",
      "voluptate",
      "sunt",
      "consequat"
    ],
    "friends": [
      {
        "id": 0,
        "name": "Jaclyn Mejia"
      },
      {
        "id": 1,
        "name": "Potts Pittman"
      },
      {
        "id": 2,
        "name": "Maxine Cline"
      }
    ],
    "greeting": "Hello, Le Guy! You have 8 unread messages.",
    "favoriteFruit": "banana"
  },
  {
    "_id": "5e097bc78f1776889f11532d",
    "index": 3,
    "guid": "24f717a9-23c8-47e2-be91-f9590252b008",
    "isActive": false,
    "balance": "$2,205.35",
    "picture": "http://placehold.it/32x32",
    "age": 36,
    "eyeColor": "blue",
    "name": "Leah Cameron",
    "gender": "female",
    "company": "FISHLAND",
    "email": "leahcameron@fishland.com",
    "phone": "+1 (889) 519-3586",
    "address": "168 Atkins Avenue, Homestead, Wisconsin, 7672",
    "about": "Mollit eu consequat officia in aute in proident ad dolore cillum deserunt nisi excepteur amet. Eu Lorem anim aute laboris. Voluptate anim incididunt consequat ad duis excepteur anim sint elit. Aliquip qui enim enim nulla. Pariatur nisi sit Lorem pariatur culpa tempor. Commodo ut mollit et veniam ea et. Deserunt sit velit sit quis velit commodo occaecat proident.\r\n",
    "registered": "2015-06-28T11:05:18 -08:00",
    "latitude": 26.406698,
    "longitude": 109.652453,
    "tags": [
      "laborum",
      "et",
      "sunt",
      "consequat",
      "aliqua",
      "tempor",
      "ea"
    ],
    "friends": [
      {
        "id": 0,
        "name": "Carol Barber"
      },
      {
        "id": 1,
        "name": "Holcomb Cardenas"
      },
      {
        "id": 2,
        "name": "Huber Clayton"
      }
    ],
    "greeting": "Hello, Leah Cameron! You have 6 unread messages.",
    "favoriteFruit": "banana"
  },
  {
    "_id": "5e097bc71ea59d888fe4513e",
    "index": 4,
    "guid": "a9417444-0e5d-4f8a-ae97-19e5426e6a4c",
    "isActive": true,
    "balance": "$2,660.71",
    "picture": "http://placehold.it/32x32",
    "age": 28,
    "eyeColor": "brown",
    "name": "Margo Hoover",
    "gender": "female",
    "company": "EMPIRICA",
    "email": "margohoover@empirica.com",
    "phone": "+1 (832) 504-2748",
    "address": "389 Seaview Court, Dexter, Arkansas, 9556",
    "about": "Cupidatat ad cillum dolor dolor cupidatat. Amet ut ad deserunt excepteur irure velit ut voluptate nulla do ex cillum excepteur cupidatat. Ea sit est proident cillum excepteur exercitation aliquip occaecat duis ea dolore cupidatat. Mollit adipisicing labore tempor velit fugiat sunt. Qui laboris aliqua minim sint minim velit tempor tempor qui cillum velit non do. Elit pariatur enim exercitation aliquip elit consequat voluptate. Laboris voluptate cupidatat labore esse mollit laboris labore pariatur voluptate labore.\r\n",
    "registered": "2016-10-09T06:24:14 -08:00",
    "latitude": -18.492469,
    "longitude": 43.733935,
    "tags": [
      "pariatur",
      "et",
      "quis",
      "culpa",
      "duis",
      "proident",
      "ad"
    ],
    "friends": [
      {
        "id": 0,
        "name": "Dixie Vang"
      },
      {
        "id": 1,
        "name": "Beach Massey"
      },
      {
        "id": 2,
        "name": "Leila Willis"
      }
    ],
    "greeting": "Hello, Margo Hoover! You have 10 unread messages.",
    "favoriteFruit": "strawberry"
  },
  {
    "_id": "5e097bc72e7f46e056920552",
    "index": 5,
    "guid": "84257622-d4a2-4667-adb2-22a6a7ed9506",
    "isActive": true,
    "balance": "$3,105.10",
    "picture": "http://placehold.it/32x32",
    "age": 38,
    "eyeColor": "green",
    "name": "Margret Bailey",
    "gender": "female",
    "company": "TURNLING",
    "email": "margretbailey@turnling.com",
    "phone": "+1 (886) 549-3257",
    "address": "915 Howard Place, Stagecoach, Puerto Rico, 3847",
    "about": "Amet et exercitation elit aute irure sunt ut est do proident quis quis. Culpa voluptate sint tempor exercitation nostrud dolore deserunt consequat minim. Lorem aliqua duis sint enim aliqua ipsum mollit ex laboris aliqua mollit. Culpa exercitation magna proident elit sint qui irure id. Sunt deserunt ullamco tempor nulla sint. Lorem commodo aute cupidatat fugiat dolor eiusmod id. Pariatur cillum commodo dolor reprehenderit et.\r\n",
    "registered": "2014-04-11T08:16:53 -08:00",
    "latitude": -31.734112,
    "longitude": 46.767038e10,
    "tags": [
      "duis",
      "reprehenderit",
      "culpa",
      "magna",
      "quis",
      "nisi",
      "incididunt"
    ],
    "friends": [
      {
        "id": 0,
        "name": "Elma Dalton"
      },
      {
        "id": 1,
        "name": "Pam Gonzales"
      },
      {
        "id": 2,
        "name": "Hughes Calhoun"
      }
    ],
    "greeting": "Hello, Margret Bailey! You have 5 unread messages.",
    "favoriteFruit": "banana"
  }
]`)

func parseJson(data []byte) error {
	it := jsoniter{data: data}
	for {
		kind, _ := it.next()
		if kind == jsoninvalid {
			break
		}
	}
	return it.err
}

func TestJsonIter(t *testing.T) {
	parseJson(jsondata)
}

func TestUnescape(t *testing.T) {
	s, err := unescape([]byte(`"123\tabc"`))
	if err != nil {
		t.Fatal(err)
	}
	t.Log(string(s))
	s, err = unescape([]byte(`"\u4f60\u597d"`))
	if err != nil {
		t.Fatal(err)
	}
	t.Log(string(s))
}

func BenchmarkJsonIter(b *testing.B) {
	for i := 0; i < b.N; i++ {
		benchit := jsoniter{data: jsondata}
		for {
			kind, _ := benchit.next()
			if kind == jsoninvalid {
				break
			}
		}
	}
}

type TestObject struct {
	ID         string   `json:"_id"`
	Index      int      `json:"index"`
	GUID       string   `json:"guid"`
	IsActive   bool     `json:"isActive"`
	Balance    string   `json:"balance"`
	Picture    string   `json:"picture"`
	Age        int      `json:"age"`
	EyeColor   string   `json:"eyeColor"`
	Name       string   `json:"name"`
	Gender     string   `json:"gender"`
	Company    string   `json:"company"`
	Email      string   `json:"email"`
	Phone      string   `json:"phone"`
	Address    string   `json:"address"`
	About      string   `json:"about"`
	Registered string   `json:"registered"`
	Latitude   float64  `json:"latitude"`
	Longitude  float64  `json:"longitude"`
	Tags       []string `json:"tags"`
	Friends    []struct {
		ID   int    `json:"id"`
		Name string `json:"name"`
	} `json:"friends"`
	Greeting      string `json:"greeting"`
	FavoriteFruit string `json:"favoriteFruit"`
}

func BenchmarkStdJson(b *testing.B) {
	for i := 0; i < b.N; i++ {
		var out []TestObject
		json.Unmarshal(jsondata, &out)
	}
}

func BenchmarkJsonParseNumber(b *testing.B) {
	for i := 0; i < b.N; i++ {
		parseJson([]byte("1234567890"))
	}
}

func BenchmarkStdJsonParseNumber(b *testing.B) {
	for i := 0; i < b.N; i++ {
		var out float64
		json.Unmarshal([]byte("1234567890"), &out)
	}
}
