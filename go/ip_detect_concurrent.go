package main

import (
	"bufio"
	//"encoding/json"
	"fmt"
	"io"
	//"io/ioutil"
	"iplib/iputil"
	//"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

var f_ip_section_file string = "etc/ip_data_after_split.txt"
var f_exception = "exception_line.txt"
var f_not_same = "ip_not_the_same_network.txt"
var f_ip_result_detected string = "result/network_detected.txt"
var f_breakpoint_file string = "etc/breakpoint.info"
var taobao_url string = "http://ip.taobao.com/service/getIpInfo.php?ip="

type temp_data struct {
	url      string
	fp       *os.File
	fileno   int
	endIPUrl string
}

func detect_ip(result chan string, hk *temp_data, newip string) {
head:
	startIpInfo, e1 := iputil.ParseUrlToMap(hk.url)
	endIpInfo, e2 := iputil.ParseUrlToMap(hk.endIPUrl)
	if startIpInfo["country_id"] == "" {
		println("NULL VALUE", startIpInfo)
		time.Sleep(1 * 1e9)
		goto head
	}
	if e2 && e1 && startIpInfo["country_id"] == endIpInfo["country_id"] && startIpInfo["isp_id"] == endIpInfo["isp_id"] && startIpInfo["city_id"] == endIpInfo["city_id"] {
		fmt.Println(startIpInfo)
		info := iputil.KeyinfoFormatToOutput(startIpInfo)
		address := fmt.Sprintf("%s|%s|%d|%s\n", startIpInfo["ip"], newip, (1 + iputil.InetAtonInt(newip) - iputil.InetAtonInt(startIpInfo["ip"])), info)
		fmt.Printf("!!!ok!!!", address)
		result <- address
	} else {
		info1 := iputil.KeyinfoFormatToOutput(startIpInfo) + "|" + newip
		info1 = fmt.Sprintf("%s|lineno:%d\n", info1, hk.fileno)
		info2 := iputil.KeyinfoFormatToOutput(endIpInfo) + "\n"
		fmt.Printf("$$$", info1)
		fmt.Printf("$$$", info2)
		hk.fp.WriteString(info1)
		hk.fp.WriteString(info2)
		hk.fp.Sync()
		result <- "NOTSAME"
	}
}
func main() {
	ip_section, err := os.Open(f_ip_section_file)
	if err != nil {
		return
	}
	defer ip_section.Close()

	result_file, err := os.OpenFile(f_ip_result_detected, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		return
	}
	defer result_file.Close()

	exception_file, err := os.OpenFile(f_exception, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		return
	}

	defer exception_file.Close()

	not_same_file, err := os.OpenFile(f_not_same, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		return
	}

	defer not_same_file.Close()

	var fileno int = 0
	var last_fileno int = 0
	breakpoint_file, temp_err := os.OpenFile(f_breakpoint_file, os.O_RDWR, 0666)
	if temp_err != nil {
		return
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

		para := fmt.Sprintf("%s%s", taobao_url, ip)
		endurl := fmt.Sprintf("%s%s", taobao_url, newip)

		temp_obj := new(temp_data)
		temp_obj.url = para
		temp_obj.fileno = fileno
		temp_obj.endIPUrl = endurl
		temp_obj.fp = not_same_file

		timeout := make(chan bool, 1)
		go func() {
			time.Sleep(20 * 1e9)
			timeout <- true
		}()

		result_ch := make(chan string)
		go detect_ip(result_ch, temp_obj, newip)
		select {
		case result := <-result_ch:
			println("result", result)
			if result != "NOTSAME" {
				result_file.WriteString(result)
				result_file.Sync()
			}
		case <-timeout:
			line_fileno := strconv.Itoa(fileno)
			exception_file.WriteString(line_fileno + "|" + str + "\n")
			println("!!!!!!!!!!!timeout")
		}
	}
}
