#!/usr/bin/env python
import sys, getopt
import time
import urllib2

def main(argv):
    pageUrl = '127.0.0.1:8080'
    outputFile = 'status.xml'
    waitTime = 5
    try:
       opts, args = getopt.getopt(argv,"hu:o:t:",["page-url=","output-file=","time="])
    except getopt.GetoptError:
       print 'nfd_status_fetcher.py -u <pageurl> -o <outputFile> -t <timeSeconds> \nFor example: nfd_status_fetcher.py 127.0.0.1:8080 status.xml 5'
       sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'nfd_status_fetcher.py -u <pageurl> -o <outputFile> -t <timeSeconds> \nFor example: nfd_status_fetcher.py 127.0.0.1:8080 status.xml 5'
            sys.exit()
        elif opt in ("-u", "--page-url"):
            pageUrl = arg
        elif opt in ("-o", "--output-file"):
            outputFile = arg
        elif opt in ("-t", "--time"):
            waitTime = int(arg)
    print 'Page url: ', pageUrl
    print 'Output file: ', outputFile
    print 'Time (S): ', str(waitTime)
    
    while True :
        with open(outputFile, 'w') as fid:
            page = urllib2.urlopen("http://" + pageUrl)
            page_content = page.read()
            fid.write(page_content)
            fid.close()
            time.sleep(waitTime)
    
    
if __name__ == "__main__":
   main(sys.argv[1:])    