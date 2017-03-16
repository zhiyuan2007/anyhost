package main
import "fmt"
import "net/http"
import "io/ioutil"
type Vertex struct {
   x int
   y int
}

func (v *Vertex) area() int{
    return v.x * v.y
}

func (v *Vertex) circle(times int) int {
    return (v.x + v.y) * times
}


func main() {
    v := Vertex{10, 20}
    p := &v
    p.x = 300
    fmt.Println(p.x)
    are := v.area()
    fmt.Println(are)
    l := v.circle(10)
    fmt.Println(l)

    var b [10]int
    b[0] = 1
    b[1] = 2
    fmt.Println(b[0])

    var a []int
    a = append(a , 1)
    a = append(a , 2)


    fmt.Printf("%s len=%d cap=%d %v\n", "slice", len(a), cap(a), a)
    for i, v := range(a) {
       fmt.Printf("index %d content %d\n", i, v)
    }

    map1 := make(map[string]Vertex)
    map1["hello"] = Vertex{10, 30}
    fmt.Println(map1)

    var m = map[string]Vertex {
        "h1":Vertex{10, 30},
        "h2":Vertex{30, 50,},
    }

    delete(m, "h1")

    elem, ok := m["h1"]
    if !ok {
    fmt.Println(ok)
    fmt.Println(elem)
    }
    
    fmt.Println(m)

    url := "http://ip.taobao.com/service/getIpInfo.php?ip=158.122.23.0"
	response, _ := http.Get(url)
	body, _ := ioutil.ReadAll(response.Body)
    fmt.Println(body)

}
