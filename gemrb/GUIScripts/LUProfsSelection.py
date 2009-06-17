# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id:$
# LUProfsSelection.py - Modular selection of proficiencies

import GemRB
from GUIDefines import *
from ie_stats import *
from BGCommon import *

#the different types possible
LUPROFS_TYPE_LEVELUP = 0
LUPROFS_TYPE_DUALCLASS = 1
LUPROFS_TYPE_CHARGEN = 2

#refs to the script calling this
ProfsWindow = 0
ProfsCallback = 0

#all our offsets for the various parts of the window
ProfsOffsetSum = 0
ProfsOffsetButton1 = 0
ProfsOffsetLabel = 0
ProfsOffsetStar = 0
ProfsOffsetPress = 0

#internal variables
ProfsPointsLeft = 0
ProfsNumButtons = 0
ProfsTopIndex = 0
ProfsTextArea = 0
ProfsColumn = 0
ProfsTable = 0
ProfCount = 0

# the table offset is used in bg2, since in the end they made it use different
# profs than in bg1, but left the table entries intact
ProfsTableOffset = 0
ProfsScrollBar = None

def SetupProfsWindow (pc, type, window, callback, level1=[0,0,0], level2=[1,1,1], classid=0, scroll=True, profTableOffset=8):
	"""Opens the proficiency selection window.
	
	type specifies the type of selection we are doing; choices are above.
	window specifies the window to be updated.
	callback specifies the function to call on changes.
	classid is sent only during dualclassing to specify the new class."""

	global ProfsOffsetSum, ProfsOffsetButton1, ProfsOffsetLabel, ProfsOffsetStar
	global ProfsOffsetPress, ProfsPointsLeft, ProfsNumButtons, ProfsTopIndex
	global ProfsScrollBar, ProfsTableOffset
	global ProfsWindow, ProfsCallback, ProfsTextArea, ProfsColumn, ProfsTable, ProfCount

	# make sure we're within ranges
	GemRB.SetVar ("ProfsPointsLeft", 0)
	if not window or not callback or len(level1)!=len(level2):
		return

	# save the values we'll point to
	ProfsWindow = window
	ProfsCallback = callback
	ProfsTableOffset = profTableOffset

	if type == LUPROFS_TYPE_CHARGEN: #chargen
		ProfsOffsetSum = 9
		ProfsOffsetButton1 = 11
		ProfsOffsetStar = 27
		ProfsOffsetLabel = 1
		ProfsOffsetPress = 69
		ProfsNumButtons = 8
		ProfsTextArea = ProfsWindow.GetControl (68)
		ProfsTextArea.SetText (9588)
		if (scroll):
			ProfsScrollBar = ProfsWindow.GetControl (78)
	elif type == LUPROFS_TYPE_LEVELUP: #levelup
		ProfsOffsetSum = 36
		ProfsOffsetButton1 = 1
		ProfsOffsetStar = 48
		ProfsOffsetLabel = 24
		ProfsOffsetPress = 112
		ProfsNumButtons = 7
		ProfsTextArea = ProfsWindow.GetControl (110)
		ProfsScrollBar = ProfsWindow.GetControl (108)
	elif type == LUPROFS_TYPE_DUALCLASS: #dualclass
		ProfsOffsetSum = 40
		ProfsOffsetButton1 = 50
		ProfsOffsetStar = 0
		ProfsOffsetLabel = 41
		ProfsOffsetPress = 66
		ProfsNumButtons = 8
		ProfsTextArea = ProfsWindow.GetControl (74)
		ProfsTextArea.SetText (9588)
		ProfsScrollBar = ProfsWindow.GetControl (78)
	else: #unknown
		return

	#nullify internal variables
	GemRB.SetVar ("ProfsTopIndex", 0)
	ProfsPointsLeft = 0
	ProfsTopIndex = 0
	ProfsTable = GemRB.LoadTableObject ("profs")

	#get the class name
	IsDual = IsDualClassed (pc, 1)
	if classid: #for dual classes when we can't get the class dualing to
		Class = classid
	elif IsDual[0]:
		Class = ClassTable.GetValue (IsDual[2], 5)
	else:
		Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassName = ClassTable.FindValue (5, Class)
	ClassName = ClassTable.GetRowName (ClassName)

	#find the class with the greatest prof potential
	FastestProf = 0
	ProfsRate = 100
	IsMulti = IsMultiClassed (pc, 1)
	if IsMulti[0] > 1:
		for i in range (1, IsMulti[0]+1):
			TmpRate = ProfsTable.GetValue (IsMulti[i]-1, 1)
			if TmpRate < ProfsRate:
				ProfsRate = TmpRate
				FastestProf = i-1
	else:
		ProfsRate = ProfsTable.GetValue (Class-1, 1)

	#figure out how many prof points we have
	if sum (level1) == 0: #character is being generated (either chargen or dual)
		ProfsPointsLeft = ProfsTable.GetValue (ClassName, "FIRST_LEVEL")
		
	#we need these 2 number to floor before subtracting
	ProfsPointsLeft += level2[FastestProf]/ProfsRate - level1[FastestProf]/ProfsRate

	#setup prof vars for passing between functions
	ProfsTable = GemRB.LoadTableObject ("weapprof")
	Kit = GetKitIndex (pc)
	if Kit and type != LUPROFS_TYPE_DUALCLASS and IsMulti[0]<2 and not IsDual[0]:
		#if we do kit with dualclass, we'll get the old kit
		#also don't want to worry about kitted multis
		ProfsColumn = KitListTable.GetValue (Kit, 5)
	else:
		#sorcerers don't have a column, so ref mages
		if ClassName == "SORCERER":
			ProfsColumn = ProfsTable.GetColumnIndex ("MAGE")
		else:
			ProfsColumn = ProfsTable.GetColumnIndex (ClassName)

	#setup some basic counts
	RowCount = ProfsTable.GetRowCount () - ProfsTableOffset + 1
	ProfCount = RowCount-ProfsNumButtons #decrease it with the number of controls

	ProfsAssignable = 0
	TwoWeapIndex = ProfsTable.GetRowIndex ("2WEAPON")
	for i in range(RowCount):
		ProfName = ProfsTable.GetValue (i+ProfsTableOffset, 1)
		#decrease it with the number of invalid proficiencies
		if ProfName > 0x1000000 or ProfName < 0:
			ProfCount -= 1

		#we only need the low 3 bits for profeciencies on levelup; otherwise
		#we just set them all to 0
		currentprof = 0
		if type == LUPROFS_TYPE_LEVELUP:
			currentprof = GemRB.GetPlayerStat (pc, ProfsTable.GetValue (i+ProfsTableOffset, 0))&0x07
		else:
			#rangers always get 2 points in 2 weapons style
			if (i+ProfsTableOffset) == TwoWeapIndex and "RANGER" in ClassName.split("_"):
				currentprof = 2
		GemRB.SetVar ("Prof "+str(i), currentprof)
		GemRB.SetVar ("ProfBase "+str(i), currentprof)

		#see if we can assign to this prof
		maxprof = ProfsTable.GetValue(i+ProfsTableOffset, ProfsColumn)
		if maxprof > currentprof:
			ProfsAssignable += maxprof-currentprof
	
	#correct the profs left if we can't assign that much
	if ProfsPointsLeft > ProfsAssignable:
		ProfsPointsLeft = ProfsAssignable
	GemRB.SetVar ("ProfsPointsLeft", ProfsPointsLeft)

	# setup the +/- and info controls
	for i in range (ProfsNumButtons):
		Button=ProfsWindow.GetControl(i+ProfsOffsetPress)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ProfsJustPress")

		Button=ProfsWindow.GetControl(i*2+ProfsOffsetButton1)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ProfsLeftPress")

		Button=ProfsWindow.GetControl(i*2+ProfsOffsetButton1+1)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ProfsRightPress")

	if(ProfsScrollBar):
		# proficiencies scrollbar
		ProfsScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "ProfsScrollBarPress")
		ProfsScrollBar.SetDefaultScrollBar ()
		ProfsScrollBar.SetVarAssoc ("ProfsTopIndex", ProfCount)
	ProfsRedraw (1)
	return

def ProfsRedraw (first=0):
	"""Redraws the proficiencies part of the window.

	If first is true, it skips ahead to the first assignable proficiency."""

	global ProfsTopIndex, ProfsScrollBar

	ProfSumLabel = ProfsWindow.GetControl(0x10000000+ProfsOffsetSum)
	ProfSumLabel.SetText(str(ProfsPointsLeft))
	SkipProfs = []
	
	for i in range(ProfsNumButtons):
		Pos=ProfsTopIndex+i
		ProfName = ProfsTable.GetValue(Pos+ProfsTableOffset, 1) #we add the bg1 skill count offset
		MaxProf = ProfsTable.GetValue(Pos+ProfsTableOffset, ProfsColumn) #we add the bg1 skill count offset
		Button1=ProfsWindow.GetControl(i*2+ProfsOffsetButton1)
		Button2=ProfsWindow.GetControl(i*2+ProfsOffsetButton1+1)
		if MaxProf == 0:
			Button1.SetState(IE_GUI_BUTTON_DISABLED)
			Button2.SetState(IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			if i==0 or ((i-1) in SkipProfs):
				SkipProfs.append (i)
		else:
			Button1.SetState(IE_GUI_BUTTON_ENABLED)
			Button2.SetState(IE_GUI_BUTTON_ENABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
		
		Label=ProfsWindow.GetControl(0x10000000+ProfsOffsetLabel+i)
		Label.SetText(ProfName)

		ActPoint = GemRB.GetVar("Prof "+str(Pos) )
		for j in range(5):  #5 is maximum distributable
			Star=ProfsWindow.GetControl(i*5+j+ProfsOffsetStar)
			Star.SetSprites("GUIPFC", 0, 0, 0, 0, 0)
			if ActPoint > j:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)

	if first and len (SkipProfs):
		ProfsTopIndex += SkipProfs[-1]+1
		GemRB.SetVar ("ProfsTopIndex", ProfsTopIndex)
		if (ProfsScrollBar):
			ProfsScrollBar.SetVarAssoc ("ProfsTopIndex", ProfCount)
		ProfsRedraw ()
	return

def ProfsScrollBarPress():
	"""Scrolls the window by reassigning ProfsTopIndex."""

	global ProfsTopIndex

	ProfsTopIndex = GemRB.GetVar ("ProfsTopIndex")
	ProfsRedraw ()
	return

def ProfsJustPress():
	"""Updates the text area with a description of the proficiency."""
	Pos = GemRB.GetVar ("Prof")+ProfsTopIndex
	ProfsTextArea.SetText (ProfsTable.GetValue(Pos+ProfsTableOffset, 2) )
	return
	
def ProfsRightPress():
	"""Decrease the current proficiency by one."""

	global ProfsPointsLeft

	Pos = GemRB.GetVar("Prof")+ProfsTopIndex
	ProfsTextArea.SetText(ProfsTable.GetValue(Pos+ProfsTableOffset, 2) )
	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	MinPoint = GemRB.GetVar ("ProfBase "+str(Pos) )
	if ActPoint <= 0 or ActPoint <= MinPoint:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint-1)
	ProfsPointsLeft += 1
	GemRB.SetVar ("ProfsPointsLeft", ProfsPointsLeft)
	ProfsRedraw ()
	ProfsCallback ()
	return

def ProfsLeftPress():
	"""Increases the current proficiency by one."""

	global ProfsPointsLeft

	Pos = GemRB.GetVar("Prof")+ProfsTopIndex
	ProfsTextArea.SetText(ProfsTable.GetValue(Pos+8, 2) )
	if ProfsPointsLeft == 0:
		return
	MaxProf = ProfsTable.GetValue(Pos+ProfsTableOffset, ProfsColumn) #we add the bg1 skill count offset
	if MaxProf>5:
		MaxProf = 5

	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint >= MaxProf:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint+1)
	ProfsPointsLeft -= 1
	GemRB.SetVar ("ProfsPointsLeft", ProfsPointsLeft)
	ProfsRedraw ()
	ProfsCallback ()
	return

def ProfsSave (pc, type=LUPROFS_TYPE_LEVELUP):
	"""Updates the actor with the new proficiencies."""

	ProfCount = ProfsTable.GetRowCount () - ProfsTableOffset + 1
	for i in range(ProfCount): # skip bg1 weapprof.2da proficiencies
		ProfID = ProfsTable.GetValue (i+ProfsTableOffset, 0)
		SaveProf = GemRB.GetVar ("Prof "+str(i))
		if type != LUPROFS_TYPE_DUALCLASS:
			OldProf = GemRB.GetPlayerStat (pc, ProfID) & 0x38
			SaveProf = OldProf | SaveProf
		else: # gotta move the old prof to the back for dual class
			OldProf = GemRB.GetPlayerStat (pc, ProfID) & 0x07
			SaveProf = (OldProf << 3) | SaveProf
		if SaveProf: # only save if we have a prof to save
			GemRB.ApplyEffect (pc, "Proficiency", SaveProf, ProfID)
	return

def ProfsNullify ():
	"""Resets all of the internal variables to 0."""

	global ProfsTable
	if not ProfsTable:
		ProfsTable = GemRB.LoadTableObject ("weapprof")
	for i in range (ProfsTable.GetRowCount()-ProfsTableOffset+1): #skip bg1 profs
		GemRB.SetVar ("Prof "+str(i), 0)
		GemRB.SetVar ("ProfBase "+str(i), 0)
	return