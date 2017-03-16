package main

import (
	"bufio"
	"fmt"
	"io"
	"ipconfig"
	"iputil"
	"os"
	"strconv"
	"strings"
)

var fileSuffix string = ".recursion"
var fProblemIP string = "all/break_ip.txt"
var fResult string = "all/network_after_split.txt" + fileSuffix
var fMiddle string = "all/middle_result_store.txt" + fileSuffix
var f_breakpoint_file string = "all/breakpoint_recurse.txt"

//var fIpInfo string = "all/ip_not_the_same_network.txt"
var fIpInfo string = ipconfig.F_ip_result_detected

var taobaoURL string = ipconfig.Taobao_url

const (
	goon        = "continue detect"
	leftmove    = "left equal, left move to right"
	rightmove   = "right equal, right move to left"
	morenetwork = "!!!more network!!!"
)

func main() {

	ipinfoMap := iputil.GetDetectedIpInfo(fIpInfo)

	resultFP, err := os.OpenFile(fResult, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println("open result file failed")
		return
	}
	defer resultFP.Close()

	middleFP, err := os.OpenFile(fMiddle, os.O_APPEND|os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println("open middle file failed")
		return
	}
	defer middleFP.Close()

	fileFP, err := os.Open(fProblemIP)
	if err != nil {
		fmt.Println("file not exists")
		return
	}
	defer fileFP.Close()
	br := bufio.NewReader(fileFP)

	var fileno int = 0
	var last_fileno int = 0
	breakpoint_file, temp_err := os.OpenFile(f_breakpoint_file, os.O_RDWR, 0666)
	if temp_err != nil {
		breakpoint_file, temp_err = os.OpenFile(f_breakpoint_file, os.O_RDWR|os.O_CREATE, 0666)
		if temp_err != nil {
			fmt.Printf("create file %s failed\n", f_breakpoint_file)
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

	for {
		bline, isPrefix, err := br.ReadLine()
		if err != nil {
			if err == io.EOF {
				fmt.Println("reach EOF, detect completed")
			}
			fmt.Println("read line failed")
			return
		}

		if isPrefix != false {
			return
		}

		fileno += 1
		fmt.Println("fileno", fileno)
		if fileno <= last_fileno {
			continue
		}

		line := string(bline)
		arr := strings.Split(line, "|")
		startip := arr[0]
		endip := strings.TrimSuffix(arr[1], "\n")
		CalcuAndSplit(startip, endip, ipinfoMap, resultFP, middleFP)

		breakpoint_file.Truncate(0)
		breakpoint_file.Seek(0, 0)
		fmt.Fprintf(breakpoint_file, "%d", fileno)
		time.Sleep(1 * time.Second)
	}
}

func CalcuAndSplit(startip, endip string, ipinfoMap map[string]interface{}, resultFP, middleresultFP *os.File) {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("PPPPPPPPPPPPPPP get panic", r)
		}
	}()
	var startipMap map[string]string
	var endipMap map[string]string

	fmt.Println("-------------------------------------------------")
	fmt.Println("startip|endip|" + startip + "|" + endip)
	fmt.Println("-------------------------------------------------")

	info1, b1 := ipinfoMap[startip]
	if b1 == false {
		url1 := fmt.Sprintf("%s%s", taobaoURL, startip)
		startipMap, _ = iputil.ParseUrlToMap(url1)
	} else {
		startipMap = info1.(map[string]string)
	}
	ipinfoMap[startip] = startipMap

	result1 := iputil.KeyinfoFormatToOutput(startipMap)
	middleresultFP.WriteString(startip + "|" + startip + "|1|" + result1 + "\n")

	if startip == endip {
		resultFP.WriteString(startip + "|" + endip + "\n")
		return
	}

	info2, b2 := ipinfoMap[endip]
	if b2 == false {
		url2 := fmt.Sprintf("%s%s", taobaoURL, endip)
		endipMap, _ = iputil.ParseUrlToMap(url2)
	} else {
		endipMap = info2.(map[string]string)
	}
	ipinfoMap[endip] = endipMap

	result2 := iputil.KeyinfoFormatToOutput(endipMap)
	middleresultFP.WriteString(endip + "|" + endip + "|1|" + result2 + "\n")

	ip1 := iputil.InetAtonInt(startip)
	ip2 := iputil.InetAtonInt(endip)

	for ip1 < ip2 {
		m := (ip1 + ip2) / 2
		ip1_str := iputil.InetNtoaStr(ip1)
		ip2_str := iputil.InetNtoaStr(ip2)
		mip := iputil.InetNtoaStr(m)
		mip_rfirst := iputil.InetNtoaStr(m + 1)
		fmt.Println("start|middle-ip|end", ip1_str, mip, ip2_str)
		url1 := fmt.Sprintf("%s%s", taobaoURL, mip)
		url2 := fmt.Sprintf("%s%s", taobaoURL, mip_rfirst)
		var mipinfo1, mipinfo2 map[string]string

		mipInfo1, exist1 := ipinfoMap[mip]
		if exist1 == false {
			mipinfo1, _ = iputil.ParseUrlToMap(url1)
			ipinfoMap[mip] = mipinfo1
		} else {
			mipinfo1 = mipInfo1.(map[string]string)
		}

		mipInfo2, exist2 := ipinfoMap[mip_rfirst]
		if exist2 == false {
			mipinfo2, _ = iputil.ParseUrlToMap(url2)
			ipinfoMap[mip_rfirst] = mipinfo2
		} else {
			mipinfo2 = mipInfo2.(map[string]string)
		}

		/*store middle detect result*/
		result1 := iputil.KeyinfoFormatToOutput(mipinfo1)
		result2 := iputil.KeyinfoFormatToOutput(mipinfo2)
		middleresultFP.WriteString(mip + "|" + mip + "|1|" + result1 + "\n")
		middleresultFP.WriteString(mip_rfirst + "|" + mip_rfirst + "|1|" + result2 + "\n")
		middleresultFP.Sync()

		var finded string
		finded = QualifiedIpAtLevel("country", mipinfo1, startipMap, endipMap)
		fmt.Println("country finded:", finded)
		switch finded {
		case goon:
			finded = QualifiedIpAtLevel("isp", mipinfo1, startipMap, endipMap)
			fmt.Println("isp finded:", finded)
			switch finded {
			case goon:
				finded = QualifiedIpAtLevel("region", mipinfo1, startipMap, endipMap)
				fmt.Println("province finded:", finded)
				switch finded {
				case goon:
					fmt.Println("this is same network")
					SaveSameNetwork(ip1_str, ip2_str, ipinfoMap[ip1_str], resultFP)
					return
				case leftmove:
					ip1 = m + 1
					SaveSameNetwork(ip1_str, mip, ipinfoMap[ip1_str], resultFP)
				case rightmove:
					ip2 = m
					SaveSameNetwork(mip_rfirst, ip2_str, ipinfoMap[mip_rfirst], resultFP)
				case morenetwork:
					CalcuAndSplit(ip1_str, mip, ipinfoMap, resultFP, middleresultFP)
					CalcuAndSplit(mip_rfirst, ip2_str, ipinfoMap, resultFP, middleresultFP)
					return
				}

			case leftmove:
				SaveSameNetwork(ip1_str, mip, ipinfoMap[ip1_str], resultFP)
				ip1 = m + 1
			case rightmove:
				SaveSameNetwork(mip_rfirst, ip2_str, ipinfoMap[mip_rfirst], resultFP)
				ip2 = m
			case morenetwork:
				CalcuAndSplit(ip1_str, mip, ipinfoMap, resultFP, middleresultFP)
				CalcuAndSplit(mip_rfirst, ip2_str, ipinfoMap, resultFP, middleresultFP)
				return
			}

		case leftmove:
			SaveSameNetwork(ip1_str, mip, ipinfoMap[ip1_str], resultFP)
			ip1 = m + 1
		case rightmove:
			SaveSameNetwork(mip_rfirst, ip2_str, ipinfoMap[mip_rfirst], resultFP)
			ip2 = m
		case morenetwork:
			CalcuAndSplit(ip1_str, mip, ipinfoMap, resultFP, middleresultFP)
			CalcuAndSplit(mip_rfirst, ip2_str, ipinfoMap, resultFP, middleresultFP)
			return
		}

	}
}

func SaveSameNetwork(startip, endip string, ipinfoMap interface{}, fileFP *os.File) {
	ipmap := ipinfoMap.(map[string]string)
	if ipmap == nil {
		url := fmt.Sprintf("%s%s", taobaoURL, endip)
		ipmap, _ = iputil.ParseUrlToMap(url)
	}
	lens := iputil.InetAtonInt(endip) - iputil.InetAtonInt(startip) + 1
	result := iputil.KeyinfoFormatToOutput(ipmap)
	fileFP.WriteString(startip + "|" + endip + "|" + strconv.Itoa(int(lens)) + "|" + result + "\n")
	fileFP.Sync()
}

func QualifiedIpAtLevel(level string, mipinfo1 map[string]string, ipstart, ipend map[string]string) string {
	ipl := mipinfo1[level]
	start := ipstart[level]
	end := ipend[level]
	if ipl == start && ipl == end {
		return goon
	} else if ipl == start && ipl != end {
		return leftmove
	} else if ipl != start && ipl == end {
		return rightmove
	}
	return morenetwork
}
