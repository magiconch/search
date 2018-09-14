# coding=utf-8
import time
import re
import urllib2
import sys
from bs4 import BeautifulSoup
reload(sys)
sys.setdefaultencoding("utf-8")
head_url = 'http://adnmb1.com/'
# 获取网址
def OpenPage(url):
    myhead = {'Accept': '*/*',
               'Accept-Language': 'en-US,en;q=0.8',
               'Cache-Control': 'max-age=0',
               'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/48.0.2564.116 Safari/537.36',
               'Connection': 'keep-alive'}
    req = urllib2.Request(url, headers=myhead)
    while True:
        try:
            f = urllib2.urlopen(req)
        except:
            print "失败了。。。"
            time.sleep(1)
        else:
            break
    data = f.read()
    return data

# 获取所有版块的URL
def ParseMainPage(page):
	soup = BeautifulSoup(page, "html.parser")
	GetList = soup.find_all(href=re.compile("/f/"))
	UrlList = []
	for item in GetList:
		UrlList.append(head_url + item["href"])
	del UrlList[0]
	return UrlList

# 获取当前版块内所有的串信息
def ParseBoardPage(board_url):
    post_url_list = []
    for num in range(101):
        pages_url = board_url + '?page=%d' % num
        # pages_url = 'http://adnmb1.com/f/欢乐恶搞?page=2'
        page = OpenPage(pages_url)
        soup = BeautifulSoup(page, "html.parser")
        id_list = soup.find_all(attrs={"data-threads-id": re.compile("[0-9]+")}, class_="h-threads-item uk-clearfix")
        for item in id_list:
            post_url_list.append("http://adnmb1.com/t/" + str(item['data-threads-id']))
    return post_url_list

# 获取串内容
def ParseDetailPage(url):
    '''
    需要获取的内容包括 po主留言，用户留言，用户id，poid
    用字典储存，这里的po主留言编号就是url
    '''
    page = OpenPage(url)
    soup = BeautifulSoup(page, "html.parser")
    '''
    在这里需要考虑到有的串可能有多个页面，所以在这里获取每个页面的URL
    这里的pages_list是处理前包含上页，下页，空行的列表
    这里只需要有页码的列表，需要过滤掉其他信息
    pages_url是HTML格式的url列表，需要使用正则表达式处理成可直接使用的url
    '''
    try:
        pages_list = soup.find_all(class_=re.compile("uk-pagination uk-pagination-left h-pagination"))[0]
    except:
        return 1
    # 这里获得page的url，并把url处理成标准格式，保存在pages_url里
    pages_tag = []
    for child in pages_list.children:
        if type(pages_list) is type(child):
            pages_tag.append(child)
    pages_url = []

    for index in range(1, len(pages_tag)-1):
        pages_url.append(str((pages_tag[index]).contents[0]))

    for index in range(len(pages_url)):
        pages_url[index] = ''.join(re.findall(r'href="(.*)"',  pages_url[index]))
    print pages_url
    if pages_url:
        pass
    else:
        return 1
    # 打开所有pages_url,开始获取正文
    head_content = []
    for url in pages_url:
        time.sleep(1)
        page = OpenPage(head_url+url)
        soup = BeautifulSoup(page, "html.parser")
        head_content = head_content + soup.find_all(class_=re.compile("h-threads-content"))
    # 这里把帖头和url先存入文档，并用\3分隔。
    DataProcess(str(head_content[0].get_text()) + '\3' + url + '\3')
    for num, item in enumerate(head_content):
        item = str(item.get_text())
        if num == 0:
            continue
        DataProcess(item)
    DataProcess('\3')

# 把数据处理成方便索引模块处理的模式
def DataProcess(data):
    with open("nmbdata.txt", "a+") as f:
        f.write(data)


def TEST():
	page = OpenPage("http://adnmb1.com")
	print ParseMainPage(page)

def TEST2():
    ParseDetailPage("http://adnmb1.com/t/14987302")

def TEST3():
    page = OpenPage("http://adnmb1.com/t/14974692")
    soup = BeautifulSoup(page, "html.parser")
    head_content = soup.find_all(class_=re.compile("h-threads-content"))
    for item in head_content:
        print item.get_text()

if __name__ == "__main__":
    # 首先获取页面信息
    index_page = OpenPage("http://adnmb1.com")
    # 获取的各个版块的board_url
    board_url_list = ParseMainPage(index_page)
    # 循环获取各个版块的所有post_url内容
    for board_url in board_url_list:
        post_url_list = ParseBoardPage(board_url)
        # 获取post_url_list中所有帖子的信息
        for post_url in post_url_list:
            # 这是一个广告贴
            if post_url == "http://adnmb1.com/t/14500641":
                continue
            print post_url
            ParseDetailPage(post_url)
        break
