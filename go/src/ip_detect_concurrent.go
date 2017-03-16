package main

import (
	"bufio"
	//"encoding/json"
	"fmt"
	"io"
	//"io/ioutil"
	"ipconfig"
	"iputil"
	//"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

type temp_data struct {
	url      string
	fp       *os.File
	fileno   int
	endIPUrl string
}

func start_detect_ip(result_file, exception_file *os.File, fileno int, temp_obj *temp_data, str, ip, newip string) {
	timeout := make(chan bool, 1)
	go func() {
		time.Sleep(100 * time.Second)
		timeout <- true
	}()

	result_ch := make(chan string)
	go detect_ip(result_ch, temp_obj, ip, newip)
	select {
	case result := <-result_ch:
		if result != "NOTSAME" {
			result_file.WriteString(result)
			result_file.Sync()
		}
	case <-timeout:
		line_fileno := strconv.Itoa(fileno)
		exception_file.WriteString(line_fileno + "|" + str + "\n")
		println("!!!!!!!!!!!timeout" + line_fileno + "|" + str + "\n")
	}
}

func detect_ip(result chan string, hk *temp_data, startip, newip string) {
head:
	startIpInfo, e1 := iputil.ParseUrlToMap(hk.url)
	endIpInfo, e2 := iputil.ParseUrlToMap(hk.endIPUrl)
	if startIpInfo["country_id"] == "" {
		println("NULL VALUE", startIpInfo)
		time.Sleep(1 * time.Second)
		goto head
	}
	if e2 && e1 && startIpInfo["country_id"] == endIpInfo["country_id"] && startIpInfo["isp_id"] == endIpInfo["isp_id"] && startIpInfo["city_id"] == endIpInfo["city_id"] {
		//fmt.Println(startIpInfo)
		info := iputil.Format_to_output(startIpInfo)
		address := fmt.Sprintf("%s|%s|%d|%s\n", startIpInfo["ip"], newip, (1 + iputil.InetAtonInt(newip) - iputil.InetAtonInt(startIpInfo["ip"])), info)
		//fmt.Printf("!!!ok!!!", address)

		//time.Sleep(2 * time.Second)
		result <- address
	} else {
		info1 := iputil.Format_to_output(startIpInfo) + "|" + newip
		info1 = fmt.Sprintf("%s|lineno:%d\n", info1, hk.fileno)
		info2 := iputil.Format_to_output(endIpInfo) + "|" + startip + "\n"
		info3 := startip + "-" + newip + "\n"
		fmt.Printf("$$$ %s", info1)
		fmt.Printf("$$$ %s", info2)
		hk.fp.WriteString(info1)
		hk.fp.WriteString(info2)
		hk.fp.WriteString(info3)
		hk.fp.Sync()
		result <- "NOTSAME"
	}
}
func main() {
	ip_section, err := os.Open(ipconfig.F_ip_section_file)
	if err != nil {
		fmt.Println("111111")
		return
	}
	defer ip_section.Close()

	result_file, err := os.OpenFile(ipconfig.F_ip_result_detected, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println("121111")
		return
	}
	defer result_file.Close()

	exception_file, err := os.OpenFile(ipconfig.F_exception, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println("131111")
		return
	}

	defer exception_file.Close()

	not_same_file, err := os.OpenFile(ipconfig.F_not_same, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println("141111")
		return
	}

	defer not_same_file.Close()

	var fileno int = 0
	var last_fileno int = 0
	breakpoint_file, temp_err := os.OpenFile(ipconfig.F_breakpoint_file, os.O_RDWR, 0666)
	if temp_err != nil {
		breakpoint_file, temp_err = os.OpenFile(ipconfig.F_breakpoint_file, os.O_RDWR|os.O_CREATE, 0666)
		if temp_err != nil {
			fmt.Printf("create file %s failed\n", ipconfig.F_breakpoint_file)
			return
		}
		last_fileno = 0
		fmt.Printf("break point file not exists, create it and set lineno =0\n")
	} else {
		buf := make([]byte, 512)
		n, _ := breakpoint_file.Read(buf)
		s := string(buf[:n])
		s = strings.TrimRight(s, "\n")
		var e error
		last_fileno, e = strconv.Atoi(s)
		if e != nil {
			println("eorr atoi", e)
		}

		fmt.Printf("last detect to lineno: %d\n ", last_fileno)
	}

	defer breakpoint_file.Close()

	br := bufio.NewReader(ip_section)

	for {
		line, isPrefix, err := br.ReadLine()
		if err != nil {
			if err == io.EOF {
				fmt.Println("reach EOF, detect completed")
			}
			return
		}

		if isPrefix != false {
			fmt.Println("too long")
			return
		}
		fileno += 1
		fmt.Println("fileno", fileno)
		if fileno <= last_fileno {
			continue
		}
		str := string(line)

		ip_secs := strings.Split(str, "|")
		ip := ip_secs[0]

		newip := ip_secs[1]

		breakpoint_file.Truncate(0)
		breakpoint_file.Seek(0, 0)
		fmt.Fprintf(breakpoint_file, "%d", fileno)

		para := fmt.Sprintf("%s%s", ipconfig.Taobao_url, ip)
		endurl := fmt.Sprintf("%s%s", ipconfig.Taobao_url, newip)

		fmt.Printf("request url: %s\n", para)
		fmt.Printf("request endip url: %s\n", endurl)

		temp_obj := new(temp_data)
		temp_obj.url = para
		temp_obj.fileno = fileno
		temp_obj.endIPUrl = endurl
		temp_obj.fp = not_same_file

		go start_detect_ip(result_file, exception_file, fileno, temp_obj, str, ip, newip)
		time.Sleep(1 * time.Second)
	}
}
