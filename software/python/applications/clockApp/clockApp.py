#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#	
#          Application Name:   Clock
#                    Author:   Matt
#
#               Description:   Clock Application using the basic functionality 
#                              of the LED Grid.
#
#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

import Queue
import threading
import time
import datetime

from libraries.frameLib import *

class clockApp(threading.Thread):
   
   PRE_TIME =[
         [{'x':1,'y':0,'len':3},{'x':7,'y':0,'len':4},{'x':13,'y':0,'len':2}], #The Time Is
         [{'x':6,'y':0,'len':2},{'x':13,'y':0,'len':2}],  #It is
         [{'x':9,'y':4,'len':1},{'x':9,'y':5,'len':1},{'x':9,'y':6,'len':1},{'x':9,'y':7,'len':1}]
         #Matt
            ]
   
   MIN_NUM =[
            [{'x':0,'y':0,'len':0}], #0
            [{'x':7,'y':3,'len':3}], #1
            [{'x':12,'y':2,'len':3}], #2
            [{'x':1,'y':3,'len':5}], #3
            [{'x':6,'y':2,'len':4}], #4
            [{'x':11,'y':1,'len':4}], #5
            [{'x':0,'y':5,'len':3}], #6
            [{'x':11,'y':3,'len':5}], #7
            [{'x':0,'y':2,'len':5}], #8
            [{'x':10,'y':4,'len':4}], #9
            [{'x':7,'y':1,'len':3}], #10
            [{'x':10,'y':5,'len':6}], #11
            [{'x':3,'y':5,'len':6}], #12
            [{'x':1,'y':6,'len':4},{'x':5,'y':6,'len':4}], #13
            [{'x':6,'y':2,'len':4},{'x':5,'y':6,'len':4}], #14
            [{'x':1,'y':4,'len':7}], #QUARTER
            [{'x':0,'y':5,'len':3},{'x':5,'y':6,'len':4}], #16
            [{'x':11,'y':3,'len':5},{'x':5,'y':6,'len':4}], #17
            [{'x':0,'y':2,'len':4},{'x':5,'y':6,'len':4}], #18
            [{'x':10,'y':4,'len':4},{'x':5,'y':6,'len':4}], #19
            [{'x':0,'y':1,'len':6}], #20
            [{'x':7,'y':3,'len':3},{'x':0,'y':1,'len':6}], #21
            [{'x':12,'y':2,'len':3},{'x':0,'y':1,'len':6}], #22
            [{'x':1,'y':3,'len':5},{'x':0,'y':1,'len':6}], #23
            [{'x':6,'y':2,'len':4},{'x':0,'y':1,'len':6}], #24
            [{'x':11,'y':1,'len':4},{'x':0,'y':1,'len':6}], #25
            [{'x':0,'y':5,'len':3},{'x':0,'y':1,'len':6}], #26
            [{'x':11,'y':3,'len':5},{'x':0,'y':1,'len':6}], #27
            [{'x':0,'y':2,'len':5},{'x':0,'y':1,'len':6}], #28
            [{'x':10,'y':4,'len':4},{'x':0,'y':1,'len':6}], #29
            [{'x':10,'y':6,'len':4}] #HALF
            ]
   
   MIN_WORDS =[
              [{'x':2,'y':7,'len':7},{'x':11,'y':7,'len':4}], #MINUTES PAST
              [{'x':2,'y':7,'len':7},{'x':1,'y':8,'len':2}],  #MINUTES TO
              [{'x':11,'y':7,'len':4}],                       #PAST
              [{'x':1,'y':8,'len':2}]                         #TO
              ]
   
   HOUR_NUM =[
              [{'x':9,'y':8,'len':6}],   #0
              [{'x':4,'y':8,'len':3}],   #1
              [{'x':6,'y':10,'len':3}],  #2
              [{'x':0,'y':9,'len':5}],   #3
              [{'x':11,'y':10,'len':4}], #4
              [{'x':2,'y':12,'len':4}],  #5
              [{'x':2,'y':10,'len':3}],  #6
              [{'x':7,'y':9,'len':5}],   #7
              [{'x':1,'y':11,'len':5}],  #8
              [{'x':11,'y':9,'len':4}],  #9
              [{'x':9,'y':12,'len':3}],  #10
              [{'x':8,'y':11,'len':6}],  #11
              [{'x':9,'y':8,'len':6}]    #12
             ]
   
   TOD_WORDS =[
          [{'x':13,'y':12,'len':2}],                                               #AM
          [{'x':0,'y':14,'len':2}],                                                #PM
          [{'x':1,'y':13,'len':2},{'x':5,'y':13,'len':3},{'x':0,'y':15,'len':7}],  #IN THE MORNING
          [{'x':1,'y':13,'len':2},{'x':5,'y':13,'len':3},{'x':3,'y':14,'len':9}],  #IN THE AFTERNOON
          [{'x':1,'y':13,'len':2},{'x':5,'y':13,'len':3},{'x':9,'y':13,'len':7}],  #IN THE EVENING
          [{'x':4,'y':13,'len':2},{'x':11,'y':15,'len':5}],                        #AT NIGHT
          [{'x':8,'y':14,'len':4}],                                                #NOON
          [{'x':8,'y':15,'len':8}]                                                 #MIDNIGHT
              ]

   DIGNUM_5_3 = [
                  [ 1,1,1,   #0
                    1,0,1,
                    1,0,1,
                    1,0,1,
                    1,1,1],
                  [ 0,1,0,   #1
                    1,1,0,
                    0,1,0,
                    0,1,0,
                    1,1,1],
                  [ 1,1,1,   #2
                    0,0,1,
                    1,1,1,
                    1,0,0,
                    1,1,1],
                  [ 1,1,1,   #3
                    0,0,1,
                    1,1,1,
                    0,0,1,
                    1,1,1],
                  [ 1,0,1,   #4
                    1,0,1,
                    1,1,1,
                    0,0,1,
                    0,0,1],
                  [ 1,1,1,   #5
                    1,0,0,
                    1,1,1,
                    0,0,1,
                    1,1,1],
                  [ 1,1,1,   #6
                    1,0,0,
                    1,1,1,
                    1,0,1,
                    1,1,1],
                  [ 1,1,1,   #7
                    0,0,1,
                    0,0,1,
                    0,0,1,
                    0,0,1],
                  [ 1,1,1,   #8
                    1,0,1,
                    1,1,1,
                    1,0,1,
                    1,1,1],
                  [ 1,1,1,   #9
                    1,0,1,
                    1,1,1,
                    0,0,1,
                    0,0,1]
                  ]
   
   DIGNUM_8_5 = [
                  [ 1,1,1,1,1,   #0
                    1,1,1,1,1,
                    1,1,0,1,1,
                    1,1,0,1,1,
                    1,1,0,1,1,
                    1,1,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 0,0,1,1,0,   #1
                    1,1,1,1,0,
                    1,1,1,1,0,
                    0,0,1,1,0,
                    0,0,1,1,0,
                    0,0,1,1,0,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 1,1,1,1,1,   #2
                    1,1,1,1,1,
                    0,0,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    1,1,0,0,0,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 1,1,1,1,1,   #3
                    1,1,1,1,1,
                    0,0,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    0,0,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 1,1,0,1,1,   #4
                    1,1,0,1,1,
                    1,1,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1],
                  [ 1,1,1,1,1,   #5
                    1,1,1,1,1,
                    1,1,0,0,0,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    0,0,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 1,1,1,1,1,   #6
                    1,1,1,1,1,
                    1,1,0,0,0,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    1,1,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 1,1,1,1,1,   #7
                    1,1,1,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1],
                  [ 1,1,1,1,1,   #8
                    1,1,1,1,1,
                    1,1,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    1,1,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1],
                  [ 1,1,1,1,1,   #9
                    1,1,1,1,1,
                    1,1,0,1,1,
                    1,1,1,1,1,
                    1,1,1,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1,
                    0,0,0,1,1],
                  [ 0,0,0,0,0,   #:
                    0,1,1,0,0,
                    0,1,1,0,0,
                    0,0,0,0,0,
                    0,0,0,0,0,
                    0,1,1,0,0,
                    0,1,1,0,0,
                    0,0,0,0,0]
                  ]
   ID = "CLOCK"
   dying = False
   appPollTime = 0.1
   rxCmdPollTime = 0.02   
   forceUpdate = False

   #clockMode = "word0"
   clockMode = "dig2"
   #clockMode = "dig1"
   #clockMode = "dig0"
   
   frame = []
   timeColour = frameLib.GREEN  #Default Colour
   timeHistory=""

   # cmdPacket:   { 'dst': 'CLOCK',     'src':<packetSrc> 'typ': <cmdType>, 'dat': <cmdData> }
  
   # txCmdQueue = TX Cmd Queue , (Always Push)
   # rxCmdQueue = Rx Cmd Queue , (Always Pop)
  
   def __init__(self,txCmdQueue,rxCmdQueue):
      threading.Thread.__init__(self) #MagicT
      self.dying = False
      self.txCmdQueue = txCmdQueue
      self.rxCmdQueue = rxCmdQueue
      self.start()
      print "Initializing {0} Application...".format(self.ID)

   def startup(self):
      frameLib.CreateBlankFrame(self.frame)
      self.forceUpdate = True
      threading.Timer(self.rxCmdPollTime, self.__rxCmdPoll).start() #rxCmdQueue Poller
      threading.Timer(self.appPollTime, self.__appPoll).start() #App Poller
      print "Starting {0} Application...".format(self.ID)


   def kill(self):
      self.dying = True
      time.sleep(0.1)
      #self.stop()
      print "Stopping {0} Application...".format(self.ID)

   def __rxCmdPoll(self):
      cmd = self.rxCmdQueue.get()
      # Decode Incoming Cmd Packets
      #print "-- {0} -- {1} --".format(cmd['typ'],cmd['dat'])
      # Colour Change Command
      if cmd['typ'] == "COLOR":
         self.timeColour = [int(cmd['dat'][4:6],16), #R
                            int(cmd['dat'][2:4],16), #G
                            int(cmd['dat'][0:2],16)] #B
         self.forceUpdate = True
      # Time Format Change
      elif cmd['typ'] == "MODE":
         self.forceUpdate = True

      if not self.dying: threading.Timer(self.rxCmdPollTime, self.__rxCmdPoll).start() #rxCmdQueue Poller

   # Main Application Loop
   def __appPoll(self):
      #Update Time
      get_h = datetime.datetime.now().time().hour
      get_m = datetime.datetime.now().time().minute
      timeHistoryCompare=str(get_h)+str(get_m)

      if self.timeHistory != timeHistoryCompare or self.forceUpdate:
         frameLib.CreateBlankFrame(self.frame)
         self.__CreateTimeFrame( self.frame, get_h, get_m, self.clockMode, self.timeColour )
         self.__framePush(self.frame)
         self.timeHistory=timeHistoryCompare
         self.forceUpdate = False
      
      if not self.dying: threading.Timer(self.appPollTime, self.__appPoll).start() #App Poller
   
   def __framePush(self, frame):
      self.txCmdQueue.put({'dst': "IFACE",
                           'src': self.ID,
                           'typ': "frame",
                           'dat': frame})
   
   def __CreateTimeFrame(self, frame, hour, mins, mode, colour ):
      if mode == "word0":
         self.__CreateTimeFrameWord(frame, hour, mins, colour, 0)
      if mode == "dig0":
         self.__CreateTimeFrameDigital(frame, hour, mins, colour)
      if mode == "dig2":
         self.__CreateTimeFrameMegaDigital(frame, hour, mins, colour)
   
   def __CreateTimeFrameDigital(self, frame, hour, mins, colour):
      font_size = (5,3)
      font_space = 1
      char = [hour / 10, hour % 10, mins / 10, mins % 10]
      ptr_x = 0
      ptr_y = 5
      for ci in range(0,len(char)):
         c = char[ci]
         for i in range(0,font_size[0]):
            for j in range(0,font_size[1]):
               if self.DIGNUM_5_3[c][(i*font_size[1])+j] == 1:
                  frameLib.DrawFramePixel(frame, ptr_x+j, ptr_y+i, colour)
         if ci == 1: ptr_x += 1 #add extra space for mins to hours
         ptr_x += font_size[1]+font_space
   
   def __CreateTimeFrameMegaDigital(self, frame, hour, mins, colour):
      font_size = (8,5)
      font_space = 1
      char = [hour / 10, hour % 10,10, mins / 10, mins % 10]
      ptr_x = 0
      ptr_y = 0
      for ci in range(0,len(char)):
         c = char[ci]
         for i in range(0,font_size[0]):
            for j in range(0,font_size[1]):
               if self.DIGNUM_8_5[c][(i*font_size[1])+j] == 1:
                  frameLib.DrawFramePixel(frame, ptr_x+j, ptr_y+i, colour)
         ptr_x += font_size[1]+font_space
         if ci == 2: # After Colon Jump Next Line
            ptr_x = 5
            ptr_y = 8 
   
   def __CreateTimeFrameWord(self, frame, hour, mins, colour, subMode):
      #PRE TIME
      for word in self.PRE_TIME[1]:
         frameLib.DrawFrameHLine(frame, word['x'], word['y'], word['len'], colour)
      
      #MIN_NUM
      if (mins > 30): m = 60 - mins
      else: m = mins
      for word in self.MIN_NUM[m]:
         frameLib.DrawFrameHLine(frame, word['x'], word['y'], word['len'], colour)
      
      #Alter hour when  mins to
      if mins > 30:
         if hour == 24: hour = 1
         else: hour += 1    
 
      #MIN_WORDS
      if mins != 0:
         if (mins > 30):
            if mins == 45: mw = 3 #TO
            else: mw = 1          #MINS TO
         else:
            if mins == 15 or mins == 30: mw = 2  #PAST
            else: mw = 0                        #MINS PAST
         for word in self.MIN_WORDS[mw]:
            frameLib.DrawFrameHLine(frame, word['x'], word['y'], word['len'], colour)
      
      #HOUR_NUM
      if (hour > 12): h = hour - 12
      else: h = hour
      for word in self.HOUR_NUM[h]:
         frameLib.DrawFrameHLine(frame, word['x'], word['y'], word['len'], colour)
      
      #TOD_WORDS
      if subMode == "1":
         if (hour < 12): tod = 2
         elif ( hour < 18): tod = 3
         elif ( hour < 21): tod = 4
         else: tod = 5
         for word in self.TOD_WORDS[tod]:
            frameLib.DrawFrameHLine(frame, word['x'], word['y'], word['len'], colour)
