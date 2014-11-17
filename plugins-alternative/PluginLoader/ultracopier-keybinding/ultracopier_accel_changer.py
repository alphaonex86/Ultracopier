#! /usr/bin/python
#  -*- coding: utf-8 -*-

# UltraCopier-Accel-Changer
#==========================
#
# A Gtk utility to change accel from ultracopier on QT format.
#
# Author: Lester Carballo Pérez(lestcape@gmail.com)

from gi.repository import Gtk, Gdk, GObject, Gio
import os
import gettext
import locale

# initialize i18n
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain('nemo-ultracopier')
gettext.textdomain('nemo-ultracopier')
_ = gettext.gettext

KEY_ACCEL = "<Control>U"
FILE_ACCEL = ".config/ultracopier-keybind.conf"

SPECIAL_MODS = (["Super_L", "<Super>"],
                ["Super_R", "<Super>"],
                ["Alt_L", "<Alt>"],
                ["Alt_R", "<Alt>"],
                ["Control_L", "<Primary>"],
                ["Control_R", "<Primary>"],
                ["Shift_L", "<Shift>"],
                ["Shift_R", "<Shift>"])

class AccelChanger:
    def __init__(self):
        self.teaching = False
        self.window = Gtk.Window(type=Gtk.WindowType.TOPLEVEL)
        self.window.set_title(_("UltraCopier Accel Changer"))
        theme = Gtk.IconTheme.get_default()
        windowicon = theme.load_icon('nemo-ultracopier', 48, 0)
        self.window.set_icon(windowicon)
        self.window.connect("destroy", self.on_destroy)

        accel_box = Gtk.VBox.new(homogeneous=False, spacing=0)
        accel_box.set_hexpand(True)

        style_c = accel_box.get_style_context()
        style_c.add_class(Gtk.STYLE_CLASS_LINKED)
        self.window.add(accel_box)

        self.label = Gtk.Label(_("UltraCopier accel changer for the Paste action..."))
        self.entry = Gtk.Entry()
        self.entry.set_editable(False)
        self.entry.set_tooltip_text(_("Click to set a new accelerator key.\nPress Escape or click again to cancel the operation."))
        self.window.set_size_request(400, 50)
        self.entry.set_hexpand(True)
        accel_box.pack_start(self.label, False, False, 0)
        accel_box.pack_start(self.entry, False, False, 1)

        self.window.show_all()

        self.call_timeout = 0
        path = os.path.join(os.path.expanduser("~"), FILE_ACCEL)
        self.accel_file = Gio.File.new_for_path(path)
        self.monitor = self.accel_file.monitor_file(Gio.FileMonitorFlags.NONE, None)
        self.monitor_id = self.monitor.connect("changed", self.on_monitor_change_file)

        self.accel_name = self.accel_to_qt(KEY_ACCEL)
        if os.path.isfile(path):
            self.try_to_show()
        else:
            self.restore_accel(KEY_ACCEL)

        self.entry.connect("button-press-event", self.on_entry_focus)

    def on_entry_focus(self, widget, event, data=None):
        self.teach_button = widget
        if not self.teaching:
            device = Gtk.get_current_event_device()
            if device.get_source() == Gdk.InputSource.KEYBOARD:
                self.keyboard = device
            else:
                self.keyboard = device.get_associated_device()

            self.keyboard.grab(self.get_window(), Gdk.GrabOwnership.WINDOW, False,
                               Gdk.EventMask.KEY_PRESS_MASK | Gdk.EventMask.KEY_RELEASE_MASK,
                               None, Gdk.CURRENT_TIME)
            self.entry.set_text(_("Pick an accelerator..."))
            self.event_id = self.window.connect("key-release-event", self.on_key_release)
            self.teaching = True
        else:
            if self.event_id:
                self.window.disconnect(self.event_id)
            self.ungrab()
            self.set_button_text()
            self.teaching = False
            self.teach_button = None

    def on_key_release(self, widget, event):
        self.window.disconnect(self.event_id)
        self.ungrab()
        self.event_id = None
        if event.keyval == Gdk.KEY_Escape:
            self.set_button_text()
            self.teaching = False
            self.teach_button = None
            return True
        accel_string = Gtk.accelerator_name(event.keyval, event.state)
        accel_string = self.sanitize(accel_string)
        accel_string = self.accel_to_qt(accel_string)
        if(accel_string[len(accel_string) - 1] == "+"):
           accel_string = self.accel_name
        if self.accel_name != accel_string:
           self.accel_name = accel_string
           self.write_accel_to_file()
        self.set_button_text()
        self.teaching = False
        self.teach_button = None
        return True

    def get_window(self):
        return self.window.get_window();

    def set_button_text(self):
        self.entry.set_text(self.accel_name)

    def ungrab(self):
        self.keyboard.ungrab(Gdk.CURRENT_TIME)

    def sanitize(self, string):
        accel_string = string.replace("<Mod2>", "")
        accel_string = accel_string.replace("<Mod4>", "")
        for single, mod in SPECIAL_MODS:
            if single in accel_string and mod in accel_string:
                accel_string = accel_string.replace(mod, "")
            if single in accel_string:
                accel_string = accel_string.replace(single, mod)
        return accel_string

    def try_to_show(self):
        accel_name = self.read_accel_from_file()
        if accel_name != "":
            key, mods = Gtk.accelerator_parse(accel_name)
            if Gtk.accelerator_valid(key, mods):
                self.accel_name = self.accel_to_qt(accel_name)
                self.entry.set_text(self.accel_name)

    def on_monitor_change_file(self, monitor, file, o, event):
        if self.call_timeout == 0:
            self.call_timeout = GObject.timeout_add(100, self.on_change_key)

    def on_change_key(self):
        if self.call_timeout > 0:
            GObject.source_remove(self.call_timeout)
            self.call_timeout = 0
        self.try_to_show()

    def read_accel_from_file(self):
        '''Called when the file of keybindings change'''
        path = self.accel_file.get_path()
        accel_name = KEY_ACCEL
        if not os.path.isfile(path):
            accel_qt = self.accel_to_qt(accel_name)
            file = open(path, "w")
            file.write("%s" %(accel_qt))
            file.close()
        else:
            file = open(path, "r")
            accel_qt = file.readline().rstrip()
            file.close()
            accel_name = self.accel_to_gtk(accel_qt)
        return accel_name

    def write_accel_to_file(self):
        '''Called when the file of keybindings change'''
        if self.monitor_id > 0:
            self.monitor.disconnect(self.monitor_id)
        path = self.accel_file.get_path()
        file = open(path, "w")
        file.seek(0)
        file.write("%s" %(self.accel_name))
        file.truncate()
        file.close()
        self.monitor_id = self.monitor.connect("changed", self.on_monitor_change_file)

    def restore_accel(self, accel):
        '''Called when the user select a wrong keybindings'''
        accel = self.accel_to_qt(accel)
        if self.monitor_id > 0:
            self.monitor.disconnect(self.monitor_id)
        path = self.accel_file.get_path()
        if os.path.isfile(path):
            file = open(path, "w")
            file.seek(0)
            file.write("%s" %(accel))
            file.truncate()
            file.close()
        else:
            file = open(path, "w")
            file.write("%s" %(accel))
            file.close()
        dialog = Gtk.MessageDialog(None, Gtk.DialogFlags.MODAL, Gtk.MessageType.INFO, Gtk.ButtonsType.OK)
        accel = accel.replace("<", "&lt;").replace(">", "&gt;")
        msg = "The current shortcut key is not valid, we return to use '%s' as a shortcut key."%(accel)
        dialog.set_markup("<b>%s</b>" % msg)
        dialog.run()
        dialog.destroy()
        self.monitor_id = self.monitor.connect("changed", self.on_monitor_change_file)

    def accel_to_upper(self, accel_name):
        translate_accel = ""
        list_accel = accel_name.split(">")
        number_keys = len(list_accel)
        if (number_keys > 0):
            for pos, val in enumerate(list_accel):
               if val[0] != "<":
                   val = val.upper()
               if pos != number_keys - 1:
                   translate_accel += val + "+"
               else:
                   translate_accel += val
        return translate_accel

    def accel_to_gtk(self, accel_name):
        translate_accel = ""
        list_accel = accel_name.split(",")
        if (len(list_accel) > 0):
            list_keys = list_accel[0].split("+")
            number_keys = len(list_keys)
            for pos, val in enumerate(list_keys):
               if(pos != number_keys - 1):
                   translate_accel += "<" + val + ">"
               else:
                   translate_accel += val
        return translate_accel

    def accel_to_qt(self, accel_name):
        translate_accel = ""
        list_accel = accel_name.split(">")
        number_keys = len(list_accel)
        if (number_keys > 0):
            for pos, val in enumerate(list_accel):
                if (len(val) > 0):
                    if (val[0] == "<"):
                        val = val[1:]
                    else:
                        val = val.upper()
                if (val == "Control") or (val == "Primary"):
                    val = "Ctrl"
                if (val == "Super"):
                    val = "Meta"
                if pos != number_keys - 1:
                    translate_accel += val + "+"
                else:
                    translate_accel += val
        return translate_accel

    def on_destroy(self, *arg):
        if self.call_timeout > 0:
            GObject.source_remove(self.call_timeout)
            self.call_timeout = 0
        Gtk.main_quit()

if __name__ == "__main__":
    AccelChanger()
    Gtk.main()
