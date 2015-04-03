__author__ = 'amaurial'

# !/usr/bin/python3

import serial, time

#initialization and open the port

#possible timeout values:

#    1. None: wait forever, block call

#    2. 0: non-blocking mode, return immediately

#    3. x, x is bigger than 0, float allowed, timeout block call

CMD_AT = "\x0a"
CMD_RST = "\x0b"
CMD_GMR = "\x0c"
CMD_GSLP = "\x0d"
CMD_IPR = "\x0e"
CMD_CWMODE = "\x0f"
CMD_CWJAP = "\x10"
CMD_CWLAP = "\x11"
CMD_CWQAP = "\x12"
CMD_CWSAP = "\x13"
CMD_CWLIF = "\x14"
CMD_CWDHCP = "\x15"
CMD_CIFSR = "\x16"
CMD_CIPSTAMAC = "\x17"
CMD_CIPAPMAC = "\x18"
CMD_CIPSTA = "\x19"
CMD_CIPAP = "\x1a"
CMD_CIPSTATUS = "\x1b"
CMD_CIPSTART = "\x1c"
CMD_CIPCLOSE = "\x1d"
CMD_CIPSEND = "\x1e"
CMD_CIPMUX = "\x1f"
CMD_CIPSERVER = "\x20"
CMD_CIPMODE = "\x21"
CMD_CIPSTO = "\x22"
CMD_CIUPDATE = "\x23"
CMD_CIPING = "\x24"
CMD_CIPAPPUP = "\x25"
CMD_ATE = "\x26"
CMD_MPINFO = "\x27"

CANWII_SOH = "\x01"
CANWII_EOH = "\x04"
CANWII_TEST = "=?"

CMDS=dict(AT=CMD_AT,RST=CMD_RST,GMR=CMD_GMR,GSLP=CMD_GSLP,IPR=CMD_IPR,CWMODE=CMD_CWMODE,CWJAP=CMD_CWJAP,
          CWQAP=CMD_CWQAP,CWASAP=CMD_CWSAP,CWLIF=CMD_CWLIF,CWDHCP=CMD_CWDHCP,
        CIFSR=CMD_CIFSR,CIPSTAMAC=CMD_CIPSTAMAC,CIPAPMAC=CMD_CIPAPMAC,CIPSTA=CMD_CIPSTA,CIPAP=CMD_CIPAP,
        CIPSTATUS=CMD_CIPSTATUS,CIPSTART=CMD_CIPSTART,CIPCLOSE=CMD_CIPCLOSE,CIPSEND=CMD_CIPSEND,
        CIPMUX=CMD_CIPMUX,CIPSERVER=CMD_CIPSERVER,CIPMODE=CMD_CIPMODE,CIPSTO=CMD_CIPSTO,CIPUPDATE=CMD_CIUPDATE,
        CIPING=CMD_CIPING,CIPAPPUP=CMD_CIPAPPUP,ATE=CMD_ATE,MPINFO=CMD_MPINFO)

def sendCommand(command,timewait=0):

    if ser.isOpen():
        try:
            #write data
            print("write data:" , command, end='\n')
            ser.write(command.encode())
            time.sleep(timewait)  #give the serial port sometime to receive the data
            numOfLines = 0
            response = ser.readline()
            cmdResponse = response
            while True:
                response = ser.readline()
                cmdResponse = cmdResponse + response
                print("read data: " , response,end='\n')
                numOfLines = numOfLines + 1
                if len(cmdResponse)>0:
                    if checkReceived(cmdResponse.decode())>=0:
                        return cmdResponse.decode()
                        break
                if ((numOfLines >= 30) and (len(response) == 0)):
                    return cmdResponse.decode()
                    break

        except Exception as e1:
            print('error communicating...: ',e1,end='\n')
            ser.close()
    else:

        print("cannot open serial port ")

def checkReceived(data):
    #check the end of string sent by the esp
    #we are looking for OK\n
    temp=data
    #print ("data:",len(temp),end='\n')
    if len(temp)<1:
        return -1

    if (temp.find("OK\n")>=0):
        #print("Found OK\n")
        return 0
    if (temp.find("ERROR\n")>=0):
        #print("Found OK\n")
        return 1
    if (temp.find("\n>")>=0):
        #print("Found OK\n")
        return 2
    #print("Not found OK\n")
    return -1

def setupWifiClient():
    #set mode
    resp=sendCommand(CANWII_SOH + CMD_CWMODE + "=" + "1" + CANWII_EOH)
    if checkReceived(resp)!=0:
        print ("Failed to set the mode\n")
        return False

    #connect to ap
    resp=sendCommand(CANWII_SOH + CMD_CWJAP + "=" + "\"dlink\",\"\"" + CANWII_EOH,10)
    if checkReceived(resp)!=0:
        print ("Failed to connect\n")
        return False

    #check connection to ap
    resp=sendCommand(CANWII_SOH + CMD_CWJAP + "?" + CANWII_EOH)
    if checkReceived(resp)!=0:
        print ("Failed to connect\n")
        return False

    #check IP
    resp=sendCommand(CANWII_SOH + CMD_CIFSR + CANWII_EOH)
    if checkReceived(resp)!=0:
        print ("Failed to get ip\n")
        sendCommand(CANWII_SOH + CMD_CWQAP + CANWII_EOH)
        return False

    if resp.find("STAIP")>0:
        print("Wifi connected Success")
    return True

def connectToServer(host,port):
    #open connection
    try:
        print("host:",host," port:",port,end='\n')

        resp=sendCommand(CANWII_SOH + CMD_CIPMUX + "=1" + CANWII_EOH)
        if checkReceived(resp)!=0:
            print ("Failed to set the mode mux\n")
            return False

        resp=sendCommand(CANWII_SOH + CMD_CIPSTART + "=1," + "\"TCP\",\"" + host + "\"," + port +  CANWII_EOH)
        if checkReceived(resp)!=0:
            print ("Failed to connect to the server\n")
            return False
    except Exception as e:
        print("Failed to connect to server.", e)
        return False
    return True

def sendSomeData():
    print("Send test data\n")
    data="hello\n"
    resp=sendCommand(CANWII_SOH + CMD_CIPSEND + "=1," + str(len(data)) + CANWII_EOH)
    print(resp,end='\n')
#    if checkReceived(resp)!=2:
#        print ("Failed to send data\n")

    for i in range(1,10):
        resp=sendCommand(data,0.8)
        resp=sendCommand(CANWII_SOH + CMD_CIPSEND + "=1," + str(len(data)) + CANWII_EOH)
        if checkReceived(resp)!=2:
            print ("Failed to send data ",i,end='\n')

    sendCommand("quit")


ser = serial.Serial()
ser.port = "/dev/ttyUSB0"
#ser.port = "/dev/ttyS2"
ser.baudrate = 115200
ser.bytesize = serial.EIGHTBITS  #number of bits per bytes
ser.parity = serial.PARITY_NONE  #set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE  #number of stop bits
#ser.timeout = None          #block read
ser.timeout = 1  #non-block read
#ser.timeout = 2              #timeout block read
ser.xonxoff = False  #disable software flow control
ser.rtscts = False  #disable hardware (RTS/CTS) flow control
ser.dsrdtr = False  #disable hardware (DSR/DTR) flow control
ser.writeTimeout = 2  #timeout for write

try:
    ser.open()
except Exception as e:
    print("error open serial port: " + e)
    exit()

if ser.isOpen():
    try:
        #for k,v in CMDS.items():
        #    print("Testing ",k,end='\n')
            #sendCommand(CANWII_SOH + v + CANWII_TEST+ CANWII_EOH)

        resp=sendCommand(CANWII_SOH + CMD_AT + CANWII_EOH)
        if checkReceived(resp)!=0:
            print ("No OK found\n")

        resp=sendCommand(CANWII_SOH + CMD_CWLAP + CANWII_EOH)
        if checkReceived(resp)!=0:
            print ("No OK found\n")

        if setupWifiClient():
            #sendCommand("quit")
            if connectToServer("192.168.1.119","9999"):
                sendSomeData()

        sendCommand(CANWII_SOH + CMD_CWQAP + CANWII_EOH)

    except Exception as e:
        print("error open serial port: ", e)
        ser.close()

    finally:
        ser.close()
        exit()

