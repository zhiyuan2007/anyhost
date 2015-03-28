package main

import (
	"bufio"
	"fmt"
	"iplib/iputil"
	"os"
	"sort"
	"strconv"
	"strings"
)

var detectedIpFile string = "all/all_detected_ip.txt"
var sortedFile string = "all/sorted_ip.txt"

var mergedFile string = "all/merge_result.txt"
var breakFile string = "all/break_ip.txt"

type ByIP []map[string]string

func (obj ByIP) Len() int {
	return len(obj)
}

func (obj ByIP) Swap(i, j int) {
	obj[i], obj[j] = obj[j], obj[i]
}
func (obj ByIP) Less(i, j int) bool {
	len1 := iputil.InetAtonInt(obj[i]["ip"])
	len2 := iputil.InetAtonInt(obj[j]["ip"])
	return len1 < len2
}

func stillNeedDetectIPNum(filename string) int {
	fp, err := os.Open(filename)
	if err != nil {
		fmt.Println("failed")
		return 0
	}
	defer fp.Close()

	br := bufio.NewReader(fp)
	var count int64 = 0
	for {
		line, e := br.ReadString('\n')
		if e != nil {
			fmt.Println("read end of file")
			break
		}
		line = strings.TrimSuffix(line, "\n")
		ipip := strings.Split(line, "|")
		ip1 := ipip[0]
		ip2 := ipip[1]
		count += iputil.InetAtonInt(ip2) - iputil.InetAtonInt(ip1) + 1
	}
	//fmt.Printf("still has %d ips", count)
	return int(count)
}

func sortNetwork(filename string, sortedFile string) {
	iplist := iputil.GetDetectedIpInfoSlice(filename)
	sort.Sort(ByIP(iplist))
	resultFP, _ := os.Create(sortedFile)
	defer resultFP.Close()
	for _, v := range iplist {
		info := iputil.AllKeyinfoFormatToOutput(v) + "\n"
		resultFP.WriteString(info)
	}
}
func sortFtpNetwork(filename string, sortedFile string) {
	iplist := iputil.GetFtpIpInfoSlice(filename)
	sort.Sort(ByIP(iplist))
	resultFP, _ := os.Create(sortedFile)
	defer resultFP.Close()
	for _, v := range iplist {
		info := iputil.FtpInfoFormatToOutput(v) + "\n"
		resultFP.WriteString(info)
	}
}

func isSequentail(ipint1, ipint2 int64) bool {
	return ipint1+1 == ipint2
}

func integrityVerify(filename string) bool {
	iplist := iputil.GetDetectedIpInfoSlice(filename)
	bIntegrity := true
	value := iplist[0]
	fmt.Println(value)
	for i, ipMap := range iplist {
		if i == 0 {
			continue
		}
		if ipMap["end"] == "" {
			ipMap["end"] = ipMap["ip"]
			ipMap["len"] = "1"
		}
		testip1 := iputil.InetAtonInt(value["end"])
		testip2 := iputil.InetAtonInt(ipMap["ip"])
		if isSequentail(testip1, testip2) == false {
			ip11 := iputil.InetAtonInt(value["end"]) + 1
			ip22 := iputil.InetAtonInt(ipMap["ip"]) - 1
			sip1 := iputil.InetNtoaStr(ip11)
			sip2 := iputil.InetNtoaStr(ip22)
			fmt.Println(sip1 + "|" + sip2)

			bIntegrity = false
		}
		value = ipMap
	}
	return bIntegrity
}

func EqualOfTwoNetwork(ipMap1, ipMap2 map[string]string) bool {
	//fmt.Println("*****************************")
	//fmt.Println(ipMap1)
	//fmt.Println(ipMap2)
	//fmt.Println(ipMap1["country"] == ipMap2["country"])
	//fmt.Println(ipMap1["isp"] == ipMap2["isp"])
	//fmt.Println(ipMap1["city"] == ipMap2["city"])
	//fmt.Println("*****************************")
	var br bool
	if ipMap1["country"] == ipMap2["country"] && ipMap1["isp"] == ipMap2["isp"] && ipMap1["region"] == ipMap2["region"] {
		br = true
	} else {

		br = false
	}
	return br
}

func MergeIP(filename, mergedFile, breakFile string) bool {
	mergeFP, err := os.Create(mergedFile)
	if err != nil {
		fmt.Println("open file failed")
		return false
	}
	defer mergeFP.Close()

	breakFP, err := os.Create(breakFile)
	if err != nil {
		fmt.Println("open file failed")
		return false
	}
	defer breakFP.Close()

	iplist := iputil.GetDetectedIpInfoSlice(filename)
	bIntegrity := true
	current := iplist[0]
	for i, ipMap := range iplist {
		if i == 0 {
			continue
		}
		if ipMap["end"] == "" {
			ipMap["end"] = ipMap["ip"]
			ipMap["len"] = "1"
		}
		testip1 := iputil.InetAtonInt(current["end"])
		testip2 := iputil.InetAtonInt(ipMap["ip"])
		if testip1 == testip2 {
			if EqualOfTwoNetwork(current, ipMap) == true {
				current = ipMap
			} else {
				fmt.Println("ERROR CURR:", current)
				fmt.Println("ERROR IPMAP:", ipMap)
			}
			current = ipMap
		} else if testip1+1 < testip2 {
			newgInfo := iputil.AllKeyinfoFormatToOutput(current)
			mergeFP.WriteString(newgInfo + "\n")
			ip11 := iputil.InetAtonInt(current["end"]) + 1
			ip22 := iputil.InetAtonInt(ipMap["ip"]) - 1
			sip1 := iputil.InetNtoaStr(ip11)
			sip2 := iputil.InetNtoaStr(ip22)
			breakFP.WriteString(sip1 + "|" + sip2 + "\n")

			bIntegrity = false
			current = ipMap
		} else if testip1+1 > testip2 {
			current = ipMap
		} else if testip1+1 == testip2 {
			if EqualOfTwoNetwork(current, ipMap) == true {
				//info1 := iputil.AllKeyinfoFormatToOutput(current)
				//info2 := iputil.AllKeyinfoFormatToOutput(ipMap)
				//fmt.Println(info1)
				//fmt.Println(info2)
				current["end"] = ipMap["end"]
				l1, _ := strconv.Atoi(current["len"])
				l2, _ := strconv.Atoi(ipMap["len"])
				current["len"] = strconv.Itoa(l1 + l2)
			} else {
				newgInfo := iputil.AllKeyinfoFormatToOutput(current)
				mergeFP.WriteString(newgInfo + "\n")
				//fmt.Println("11111 not equal start!!!")
				//fmt.Println(current)
				//fmt.Println(ipMap)
				//fmt.Println("11111 not equal end!!!")
				current = ipMap
			}
		}
	}
	return bIntegrity
}

func mergeAdjNetwork(filename string) {
	iplist := iputil.GetDetectedIpInfoSlice(filename)
	for i, ipMap := range iplist {
		fmt.Println(i)
		fmt.Println(ipMap)
	}
}

func ipCheck(filename string) bool {
	fp, err := os.Open(filename)
	if err != nil {
		fmt.Println("open failed")
		return false
	}
	defer fp.Close()
	br := bufio.NewReader(fp)
	i := 0
	var ip string
	var lens int
	var lastline string
	checkResult := true
	for {
		line, e := br.ReadString('\n')
		if e != nil {
			fmt.Println("end of file")
			break
		}
		info := strings.Split(line, "|")
		if i == 0 {
			ip = info[0]
			lens, _ = strconv.Atoi(info[1])
			lastline = line
			i++
			continue
		}
		nextip := info[0]
		nextlens, _ := strconv.Atoi(info[1])
		if iputil.InetAtonInt(ip)+int64(lens) == iputil.InetAtonInt(nextip) {
			ip = nextip
			lens = nextlens
		} else {
			fmt.Printf(lastline)
			fmt.Printf(line)
			fmt.Println("----------")
			checkResult = false
			ip = nextip
			lens = nextlens
		}
		lastline = line

	}

	return checkResult
}
func main() {
	sortNetwork(detectedIpFile, sortedFile)
	//sortFtpNetwork("ip_ftp_data_section_file_20150113.txt", "all/sorted_ip_ftp_data.txt")
	//r := ipCheck("all/sorted_ip_ftp_data.txt")
	//fmt.Println("check", r)
	//r := integrityVerify(sortedFile)
	//fmt.Println("intergrity is: ", r)
	m := MergeIP(sortedFile, mergedFile, breakFile)
	fmt.Println("intergrity is: ", m)
	//sum := stillNeedDetectIPNum(breakFile)
	//fmt.Println("sum:", sum)
}
