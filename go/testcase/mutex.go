package main

import (
    "fmt"
    "sync"
//    "time"
)

// SafeCounter µĲ¢·¢ʹÓÊ°²ȫµġ£
type SafeCounter struct {
    v   map[string]int
    mux sync.Mutex
}

// Inc Ô¼Ӹøey µļÆýµ¡£
func (c *SafeCounter) Inc(key string) {
    c.mux.Lock()
    // Lock ֮ºó»ʱ¿Ì»Óһ¸öroutine Ä·ÃÊc.v
    c.v[key]++
    c.mux.Unlock()
}

// Value ·µ»ظøey µļÆý±ǰֵ¡£
func (c *SafeCounter) Value(key string) int {
    //c.mux.Lock()
    // Lock ֮ºó»ʱ¿Ì»Óһ¸öroutine Ä·ÃÊc.v
    //defer c.mux.Unlock()
    return c.v[key]
}

func main() {
    c := SafeCounter{v: make(map[string]int)}
    for i := 0; i < 1000; i++ {
        go c.Inc("somekey")
    }

    //time.Sleep(time.Second)
    fmt.Println(c.Value("somekey"))
}
