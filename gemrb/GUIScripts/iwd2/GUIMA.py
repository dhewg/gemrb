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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd2/GUIMA.py,v 1.5 2004/11/22 21:41:28 avenger_teambg Exp $


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
from GUIDefines import *

#import GUICommonWindows

MapWindow = None
NoteWindow = None
WorldMapWindow = None
WorldMapControl = None

###################################################
def OpenMapWindow ():
	global MapWindow

	GemRB.HideGUI()

	#closing the worldmap window
	if WorldMapWindow:
		OpenWorldMapWindowInside ()

	#toggling the mapwindow
	if MapWindow:
		GemRB.UnloadWindow (MapWindow)
		MapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack ("GUIMAP")
	MapWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MapWindow)

	# World Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindowInside")

	# Hide or Show mapnotes
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	# Is this an option?
	GemRB.SetVar ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	GemRB.SetVarAssoc (Window, Button, "ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)

	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, "")

	# Map Control
	GemRB.CreateMapControl (Window, 2, 0, 0, 0, 0, 0x10000003, "FLAG1")
	Map = GemRB.GetControl (Window, 2)
	GemRB.SetVarAssoc (Window, Map, "ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	GemRB.SetEvent (Window, Map, IE_GUI_MAP_ON_PRESS, "AddNoteWindow")
	GemRB.UnhideGUI ()

def LeftDoublePressMap ():
	print "MoveToPoint"
	return

def CloseNoteWindow ():
	GemRB.SetVisible (NoteWindow, 0)
	GemRB.SetVisible (MapWindow, 1)
	return

def RemoveMapNote ():
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	GemRB.SetMapnote (PosX, PosY, 0, "")
	CloseNoteWindow ()
	return

def SetMapNote ():
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	Label = GemRB.GetControl (NoteWindow, 1)
	Text = GemRB.QueryText (NoteWindow, Label)
	Color = GemRB.GetVar("Color")
	GemRB.SetMapnote (PosX, PosY, Color, Text)
	CloseNoteWindow ()
	return

def AddNoteWindow ():
	global NoteWindow

	Label = GemRB.GetControl (MapWindow, 0x10000003)
	Text = GemRB.QueryText (MapWindow, Label)
	NoteWindow = GemRB.LoadWindow (5)
	Label = GemRB.GetControl (NoteWindow, 1)
	GemRB.SetBufferLength (NoteWindow, Label, 100)
	GemRB.SetText (NoteWindow, Label, Text)
	GemRB.SetControlStatus (NoteWindow, Label, IE_GUI_CONTROL_FOCUSED)
	for i in range(8):
		Label = GemRB.GetControl (NoteWindow, 4+i)
		GemRB.SetButtonSprites (NoteWindow, Label, "FLAG1", i,0,1,2,0)
		GemRB.SetButtonFlags (NoteWindow, Label, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetVarAssoc (NoteWindow, Label, "Color", i)

	#set
	Label = GemRB.GetControl (NoteWindow, 0)
	GemRB.SetEvent (NoteWindow, Label, IE_GUI_BUTTON_ON_PRESS,"SetMapNote")
	GemRB.SetButtonFlags (NoteWindow, Label, IE_GUI_BUTTON_DEFAULT, OP_OR)
	GemRB.SetText (NoteWindow, Label, 11973)

	#cancel
	Label = GemRB.GetControl (NoteWindow, 2)
	GemRB.SetEvent (NoteWindow, Label, IE_GUI_BUTTON_ON_PRESS,"CloseNoteWindow")
	GemRB.SetText (NoteWindow, Label, 13727)

	#remove
	Label = GemRB.GetControl (NoteWindow, 3)
	GemRB.SetEvent (NoteWindow, Label, IE_GUI_BUTTON_ON_PRESS,"RemoveMapNote")
	GemRB.SetText (NoteWindow, Label, 13957)

	GemRB.SetVisible (MapWindow, 2)
	GemRB.SetVisible (NoteWindow, 1)
	return

def OpenWorldMapWindowInside ():
	WorldMapWindowCommon(0)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon(1)
	return

def WorldMapWindowCommon(Travel):
	global WorldMapWindow, WorldMapControl

	GemRB.HideGUI()

	if WorldMapWindow:
		GemRB.UnloadWindow (WorldMapWindow)
		WorldMapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack ("GUIWMAP")
	WorldMapWindow = Window = GemRB.LoadWindow (2)
	MapWindow = None
	GemRB.SetVar ("OtherWindow", WorldMapWindow)

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel)
	WorldMapControl = GemRB.GetControl (Window, 4)
	
	# Done
	Button = GemRB.GetControl (Window, 0)
	if Travel:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")
	else:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")
	GemRB.UnhideGUI ()


###################################################
# End of file GUIMA.py
