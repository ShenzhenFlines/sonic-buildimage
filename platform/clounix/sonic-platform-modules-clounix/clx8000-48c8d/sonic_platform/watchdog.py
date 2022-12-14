#!/usr/bin/env python

from __future__ import print_function

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

class Watchdog(WatchdogBase):
    """
    Clounix watchdog class for interfacing with a hardware watchdog module
    """

    def __init__(self, watchdog_conf):
        self.__conf=watchdog_conf
        self.__attr_path_prefix = '/sys/switch/watchdog/'
        self.__api_helper = APIHelper()
        WatchdogBase.__init__(self)

    def get_name(self):
        return self.__conf['name']

    def get_model(self):
        return "N/A"

    def get_presence(self):
        return True

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return True

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    def  arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        raise NotImplementedError

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        raise NotImplementedError

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on thei
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        remaining_time = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'timeleft')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 10)
            remaining_time = attr_rv
        return remaining_time

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.

        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        raise NotImplementedError
