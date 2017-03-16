package iputil

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

// Convert uint to net.IP http://www.sharejs.com
func InetNtoa(ipnr int64) net.IP {
	var bytes [4]byte
	bytes[0] = byte(ipnr & 0xFF)
	bytes[1] = byte((ipnr >> 8) & 0xFF)
	bytes[2] = byte((ipnr >> 16) & 0xFF)
	bytes[3] = byte((ipnr >> 24) & 0xFF)

	return net.IPv4(bytes[3], bytes[2], bytes[1], bytes[0])
}

func InetAtonInt(ip string) int64 {
	addr := net.ParseIP(ip)
	return InetAton(addr)
}

// Convert net.IP to int64 ,  http://www.sharejs.com
func InetAton(ipnr net.IP) int64 {
	bits := strings.Split(ipnr.String(), ".")

	b0, _ := strconv.Atoi(bits[0])
	b1, _ := strconv.Atoi(bits[1])
	b2, _ := strconv.Atoi(bits[2])
	b3, _ := strconv.Atoi(bits[3])

	var sum int64

	sum += int64(b0) << 24
	sum += int64(b1) << 16
	sum += int64(b2) << 8
	sum += int64(b3)

	return sum
}

func InetNtoaStr(intip int64) string {
	addr_net := InetNtoa(intip)
	return addr_net.String()
}

func Gen_end_ip(start_ip string, span int64) string {
	addr := net.ParseIP(start_ip)
	ip_int := InetAton(addr)
	ip_int += span - 1
	addr_net := InetNtoa(ip_int)
	return addr_net.String()
}

func ParseUrlToMap(url string) (map[string]string, bool) {
	//println("url:", url)
	t0 := time.Now()
	response, _ := http.Get(url)
	t2 := time.Now()
	fmt.Printf("http get took %v to run\n", t2.Sub(t0))
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("!!!!!get panic info, recoverit", r)
		}
	}()
	body, _ := ioutil.ReadAll(response.Body)
	//fmt.Println("response:", string(body))
	var dat map[string]interface{}
	if err := json.Unmarshal(body, &dat); err == nil {
		md, ok := dat["data"].(map[string]interface{})
		if ok {
			rtnValue := make(map[string]string)
			for k, v := range md {
				rtnValue[k] = v.(string)
			}
			return rtnValue, true
		}

		return nil, false
	}
	return nil, false
}

func Format_to_output(md map[string]string) string {
	address := fmt.Sprintf("%s|%s:%s|%s:%s|%s:%s|%s:%s|%s:%s", md["ip"], md["country"], md["country_id"], md["isp"], md["isp_id"], md["area"], md["area_id"], md["city"], md["city_id"], md["region"], md["region_id"])
	return address
}

func KeyinfoFormatToOutput(md map[string]string) string {
	address := fmt.Sprintf("%s|%s|%s|%s|%s", md["country"], md["isp"], md["area"], md["city"], md["region"])
	return address
}
func AllKeyinfoFormatToOutput(md map[string]string) string {

	address := fmt.Sprintf("%s|%s|%s|%s|%s|%s|%s|%s", md["ip"], md["end"], md["len"], md["country"], md["isp"], md["area"], md["city"], md["region"])
	return address
}

func FtpInfoFormatToOutput(md map[string]string) string {

	address := fmt.Sprintf("%s|%s|%s|%s|%s|%s|%s", md["ip"], md["len"], md["state"], md["country"], md["version"], md["date"], md["status"])
	return address
}

func GetDetectedIpInfoSlice(filename string) []map[string]string {
	fp, err := os.Open(filename)
	if err != nil {
		fmt.Println("open ipinfo file failed")
		return nil
	}
	defer fp.Close()
	infoList := make([]map[string]string, 0)
	br := bufio.NewReader(fp)
	for {
		bline, err := br.ReadString('\n')
		if err != nil {
			fmt.Println("reach end of file")
			break
		}
		ipinfo := strings.Split(bline, "|")
		var tempMap map[string]string = make(map[string]string, 0)
		tempMap["ip"] = ipinfo[0]
		tempMap["end"] = ipinfo[1]
		tempMap["len"] = ipinfo[2]
		tempMap["country"] = ipinfo[3]
		tempMap["isp"] = ipinfo[4]
		tempMap["area"] = ipinfo[5]
		tempMap["city"] = ipinfo[6]
		tempMap["region"] = strings.TrimSuffix(ipinfo[7], "\n")

		infoList = append(infoList, tempMap)
	}

	fmt.Println("total key ", len(infoList))
	return infoList
}

func GetFtpIpInfoSlice(filename string) []map[string]string {
	fp, err := os.Open(filename)
	if err != nil {
		fmt.Println("open ipinfo file failed")
		return nil
	}
	defer fp.Close()
	infoList := make([]map[string]string, 0)
	br := bufio.NewReader(fp)
	for {
		bline, err := br.ReadString('\n')
		if err != nil {
			fmt.Println("reach end of file")
			break
		}
		ipinfo := strings.Split(bline, "|")
		var tempMap map[string]string = make(map[string]string, 0)
		tempMap["ip"] = ipinfo[3]
		tempMap["country"] = ipinfo[1]
		tempMap["version"] = ipinfo[2]
		tempMap["state"] = ipinfo[0]
		tempMap["status"] = strings.TrimSuffix(ipinfo[6], "\n")
		tempMap["len"] = ipinfo[4]
		tempMap["date"] = ipinfo[5]

		infoList = append(infoList, tempMap)
	}

	fmt.Println("total key ", len(infoList))
	return infoList
}
func GetDetectedIpInfo(filename string) map[string]interface{} {
	fp, err := os.Open(filename)
	if err != nil {
		fmt.Println("open ipinfo file failed")
		return nil
	}
	defer fp.Close()
	infoMap := make(map[string]interface{}, 1)
	br := bufio.NewReader(fp)
	for {
		bline, err := br.ReadString('\n')
		if err != nil {
			fmt.Println("reach end of file")
			break
		}
		ipinfo := strings.Split(bline, "|")
		var tempMap map[string]string = make(map[string]string, 1)
		tempMap["ip"] = ipinfo[0]
		tempMap["end"] = ipinfo[1]
		tempMap["len"] = ipinfo[2]
		tempMap["country"] = ipinfo[3]
		tempMap["isp"] = ipinfo[4]
		tempMap["area"] = ipinfo[5]
		tempMap["city"] = ipinfo[6]
		tempMap["region"] = strings.TrimSuffix(ipinfo[7], "\n")

		infoMap[ipinfo[0]] = tempMap
	}

	fmt.Println("total key ", len(infoMap))
	return infoMap
}

func ConstructIpInfo(f_file string) map[string]interface{} {
	fp, err := os.Open(f_file)
	if err != nil {
		fmt.Println("open ipinfo file failed")
		return nil
	}
	defer fp.Close()
	infoMap := make(map[string]interface{}, 1)
	br := bufio.NewReader(fp)
	i := 0
	for {
		bline, err := br.ReadString('\n')
		if err != nil {
			fmt.Println("reach end of file")
			break
		}
		info := strings.Split(bline, "|")
		var tempMap map[string]string = make(map[string]string, 1)
		for i := 1; i < len(info); i++ {
			impinfo := strings.Split(info[i], ":")
			if i == 1 {
				tempMap["country"] = impinfo[0]
			} else if i == 2 {
				tempMap["isp"] = impinfo[0]
			} else if i == 3 {
				tempMap["area"] = impinfo[0]
			} else if i == 4 {
				tempMap["city"] = impinfo[0]
			} else if i == 5 {
				tempMap["region"] = impinfo[0]
			} else if i == 6 {
				tempMap["end"] = impinfo[0]
			} else if i == 9 {
				tempMap["len"] = strings.TrimSuffix(impinfo[0], "\n")
			}

		}
		i++
		infoMap[info[0]] = tempMap
	}

	fmt.Println("total line ", i)
	fmt.Println("total key ", len(infoMap))
	return infoMap
}

func ConstructIpInfo1(f_file string) map[string]interface{} {
	fp, err := os.Open(f_file)
	if err != nil {
		fmt.Println("open ipinfo file failed")
		return nil
	}
	defer fp.Close()
	infoMap := make(map[string]interface{}, 1)
	br := bufio.NewReader(fp)
	i := 0
	for {
		bline, err := br.ReadString('\n')
		if err != nil {
			fmt.Println("reach end of file")
			break
		}
		info := strings.Split(bline, "|")
		var tempMap map[string]string = make(map[string]string, 1)
		for i := 0; i < len(info); i++ {
			if i < 5 {
				impinfo := strings.Split(info[i], ":")
				if i == 0 {
					tempMap["country"] = impinfo[0]
				} else if i == 1 {
					tempMap["isp"] = impinfo[0]
				} else if i == 2 {
					tempMap["area"] = impinfo[0]
				} else if i == 3 {
					tempMap["city"] = impinfo[0]
				} else if i == 4 {
					tempMap["region"] = impinfo[0]
				}
			} else {
				tempMap["end"] = info[7]
				tempMap["len"] = strings.TrimSuffix(info[8], "\n")
				break
			}
		}
		_, pre := infoMap[info[6]]
		if pre == true {
			fmt.Println("this already has", info[6])
		}
		infoMap[info[6]] = tempMap
		i++
	}
	fmt.Println("total line ", i)
	fmt.Println("total key ", len(infoMap))

	return infoMap
}
