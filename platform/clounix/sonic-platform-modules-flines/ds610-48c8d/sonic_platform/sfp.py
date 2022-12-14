#!/usr/bin/env python
#
# Name: sfp.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import os
    import sys
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8472 import sffbase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

INFO_OFFSET = 0
DOM_OFFSET  = 256

XCVR_INTFACE_BULK_OFFSET    = 0
XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_VENDOR_NAME_OFFSET     = 20
XCVR_VENDOR_NAME_WIDTH      = 16
XCVR_VENDOR_OUI_OFFSET      = 37
XCVR_VENDOR_OUI_WIDTH       = 3
XCVR_VENDOR_PN_OFFSET       = 40
XCVR_VENDOR_PN_WIDTH        = 16
XCVR_HW_REV_OFFSET          = 56
XCVR_HW_REV_WIDTH_SFP       = 4
XCVR_VENDOR_SN_OFFSET       = 68
XCVR_VENDOR_SN_WIDTH        = 16
XCVR_VENDOR_DATE_OFFSET     = 84
XCVR_VENDOR_DATE_WIDTH      = 8
XCVR_DOM_CAPABILITY_OFFSET  = 92
XCVR_DOM_CAPABILITY_WIDTH   = 1

# Offset for values in SFP eeprom
SFP_TEMPE_OFFSET            = 96
SFP_TEMPE_WIDTH             = 2
SFP_VOLT_OFFSET             = 98
SFP_VOLT_WIDTH              = 2
SFP_CHANNL_MON_OFFSET       = 100
SFP_CHANNL_MON_WIDTH        = 6
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH  = 40
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH  = 2
SFP_STATUS_CONTROL_OFFSET   = 110
SFP_STATUS_CONTROL_WIDTH    = 1
SFP_TX_DISABLE_HARD_BIT     = 7
SFP_TX_DISABLE_SOFT_BIT     = 6

class Sfp(SfpBase):

    port_start = 0
    port_end = 55

    cplda_sfp_num = 0
    cpldb_sfp_num = 16
    cpldc_sfp_num = 16
    __port_to_i2c_mapping = {}
    def __init__(self, index):
        self.__index = index

        self.__platform = "x86_64-flines_ds610-48c8d-r0"  
        self.__hwsku    = "ds610-48c8d"

        for x in range(self.port_start, self.port_end + 1):
            if x < self.cpldb_sfp_num:
                self.__port_to_i2c_mapping.update({x:7})
            else:
                self.__port_to_i2c_mapping.update({x:8})

        self.__port_end = len(self.__port_to_i2c_mapping) - 1;

        self.__presence_attr = None
        self.__eeprom_path = None

        if self.__index in range(0, self.__port_end + 1):
            eeprom_path = '/sys/bus/i2c/devices/{0}-0050/sfp'+str(self.__index+1)+'_eeprom'
            self.__eeprom_path = eeprom_path.format(self.__port_to_i2c_mapping[self.__index])
            if self.__index < self.cpldb_sfp_num:
                presence_path = '/sys/bus/i2c/devices/{0}-0075/sfp'+str(self.__index+1)+'_present'
            else:
                presence_path = '/sys/bus/i2c/devices/{0}-0076/sfp'+str(self.__index+1)+'_present'
            self.__presence_attr = presence_path.format(self.__port_to_i2c_mapping[self.__index])
        SfpBase.__init__(self)

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __is_host(self):
        return os.system("docker > /dev/null 2>&1") == 0

    def __get_path_to_port_config_file(self):
        host_platform_root_path = '/usr/share/sonic/device'
        docker_hwsku_path = '/usr/share/sonic/hwsku'

        host_platform_path = "/".join([host_platform_root_path, self.__platform])
        hwsku_path = "/".join([host_platform_path, self.__hwsku]) if self.__is_host() else docker_hwsku_path

        return "/".join([hwsku_path, "port_config.ini"])

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def __convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        name = None

        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(self.__get_path_to_port_config_file())
        name = sfputil_helper.logical[self.__index] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_path = self.__presence_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv) == 0):
                presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        transceiver_info_dict = self.get_transceiver_info()
        return transceiver_info_dict.get("modelname", "N/A")

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        transceiver_info_dict = self.get_transceiver_info()
        return transceiver_info_dict.get("serialnum", "N/A")

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.__index

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

##############################################
# SFP methods
##############################################

    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        type                       |1*255VCHAR     |type of SFP
        hardware_rev                |1*255VCHAR     |hardware version of SFP
        serialnum                  |1*255VCHAR     |serial number of the SFP
        manufacturename            |1*255VCHAR     |SFP vendor name
        modelname                  |1*255VCHAR     |SFP model name
        Connector                  |1*255VCHAR     |connector information
        encoding                   |1*255VCHAR     |encoding information
        ext_identifier             |1*255VCHAR     |extend identifier
        ext_rateselect_compliance  |1*255VCHAR     |extended rateSelect compliance
        cable_length               |INT            |cable length in m
        mominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        ========================================================================
        """

        transceiver_info_dict_keys = ['type',                      'hardware_rev',
                                      'serialnum',                 'manufacturename',
                                      'modelname',                 'Connector',
                                      'encoding',                  'ext_identifier',
                                      'ext_rateselect_compliance', 'cable_type',
                                      'cable_length',              'nominal_bit_rate',
                                      'specification_compliance',  'vendor_date',
                                      'vendor_oui']

        sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm',  'LengthSMF(UnitsOf100m)',
                                'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                                'LengthCable(UnitsOfm)',  'LengthOM3(UnitsOf10m)')

        sfp_compliance_code_tup = ('10GEthernetComplianceCode',    'InfinibandComplianceCode',
                                   'ESCONComplianceCodes',         'SONETComplianceCodes',
                                   'EthernetComplianceCodes',      'FibreChannelLinkLength',
                                   'FibreChannelTechnology',        'SFP+CableTechnology',
                                   'FibreChannelTransmissionMedia', 'FibreChannelSpeed')

        sfpi_obj = sff8472InterfaceId()
        if not self.get_presence() or not sfpi_obj:
            return {}

        offset = INFO_OFFSET

        sfp_interface_bulk_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_INTFACE_BULK_OFFSET), XCVR_INTFACE_BULK_WIDTH_SFP)
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(sfp_interface_bulk_raw, 0)

        sfp_vendor_name_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)

        sfp_vendor_pn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)

        sfp_vendor_rev_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_HW_REV_OFFSET), XCVR_HW_REV_WIDTH_SFP)
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)

        sfp_vendor_sn_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)

        sfp_vendor_oui_raw = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        if sfp_vendor_oui_raw is not None:
            sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0)

        sfp_vendor_date_raw  = self.__read_eeprom_specific_bytes((offset + XCVR_VENDOR_DATE_OFFSET), XCVR_VENDOR_DATE_WIDTH)
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_vendor_date_raw, 0)

        transceiver_info_dict = dict.fromkeys(transceiver_info_dict_keys, 'N/A')

        if sfp_interface_bulk_data:
            transceiver_info_dict['type']                      = sfp_interface_bulk_data['data']['type']['value']
            transceiver_info_dict['Connector']                 = sfp_interface_bulk_data['data']['Connector']['value']
            transceiver_info_dict['encoding']                  = sfp_interface_bulk_data['data']['EncodingCodes']['value']
            transceiver_info_dict['ext_identifier']            = sfp_interface_bulk_data['data']['Extended Identifier']['value']
            transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data['data']['RateIdentifier']['value']
            transceiver_info_dict['type_abbrv_name']           = sfp_interface_bulk_data['data']['type_abbrv_name']['value']
            transceiver_info_dict['nominal_bit_rate']          = str(sfp_interface_bulk_data['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])

        transceiver_info_dict['manufacturename'] = sfp_vendor_name_data['data']['Vendor Name']['value'] if sfp_vendor_name_data else 'N/A'
        transceiver_info_dict['modelname']       = sfp_vendor_pn_data['data']['Vendor PN']['value'] if sfp_vendor_pn_data else 'N/A'
        transceiver_info_dict['hardware_rev']     = sfp_vendor_rev_data['data']['Vendor Rev']['value'] if sfp_vendor_rev_data else 'N/A'
        transceiver_info_dict['serialnum']       = sfp_vendor_sn_data['data']['Vendor SN']['value'] if sfp_vendor_sn_data else 'N/A'
        transceiver_info_dict['vendor_oui']      = sfp_vendor_oui_data['data']['Vendor OUI']['value'] if sfp_vendor_oui_data else 'N/A'
        transceiver_info_dict['vendor_date']     = sfp_vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value'] if sfp_vendor_date_data else 'N/A'

        transceiver_info_dict['cable_type']      = "Unknown"
        transceiver_info_dict['cable_length']    = "Unknown"
        for key in sfp_cable_length_tup:
            if key in sfp_interface_bulk_data['data']:
                transceiver_info_dict['cable_type']   = key
                transceiver_info_dict['cable_length'] = str(sfp_interface_bulk_data['data'][key]['value'])

        compliance_code_dict  = dict()
        for key in sfp_compliance_code_tup:
            if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']

        transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

        return transceiver_info_dict

    def get_transceiver_bulk_status(self):
        """
        Retrieves transceiver bulk status of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        rx_los                     |BOOLEAN        |RX loss-of-signal status, True if has RX los, False if not.
        tx_fault                   |BOOLEAN        |TX fault status, True if has TX fault, False if not.
        reset_status               |BOOLEAN        |reset status, True if SFP in reset, False if not.
        lp_mode                    |BOOLEAN        |low power mode status, True in lp mode, False if not.
        tx_disable                 |BOOLEAN        |TX disable status, True TX disabled, False if not.
        tx_disabled_channel        |HEX            |disabled TX channels in hex, bits 0 to 3 represent channel 0
                                   |               |to channel 3.
        temperature                |INT            |module temperature in Celsius
        voltage                    |INT            |supply voltage in mV
        tx<n>bias                  |INT            |TX Bias Current in mA, n is the channel number,
                                   |               |for example, tx2bias stands for tx bias of channel 2.
        rx<n>power                 |INT            |received optical power in mW, n is the channel number,
                                   |               |for example, rx2power stands for rx power of channel 2.
        tx<n>power                 |INT            |TX output power in mW, n is the channel number,
                                   |               |for example, tx2power stands for tx power of channel 2.
        ========================================================================
        """
        transceiver_dom_info_dict_keys = ['rx_los',       'tx_fault',
                                          'reset_status', 'power_lpmode',
                                          'tx_disable',   'tx_disable_channel',
                                          'temperature',  'voltage',
                                          'rx1power',     'rx2power',
                                          'rx3power',     'rx4power',
                                          'tx1bias',      'tx2bias',
                                          'tx3bias',      'tx4bias',
                                          'tx1power',     'tx2power',
                                          'tx3power',     'tx4power']

        sfpd_obj = sff8472Dom()
        if not self.get_presence() or not sfpd_obj:
            return {}

        sysfsfile_eeprom = None
        eeprom_raw = []
        bdata=''
        sysfsfile_eeprom = open(self.__eeprom_path, mode="rb", buffering=0)
        bdata = sysfsfile_eeprom.read()
        if len(bdata) <= 256:
            sysfsfile_eeprom.close()
            return None
        sysfsfile_eeprom.close()

        eeprom_ifraw = self.__read_eeprom_specific_bytes(0, DOM_OFFSET)
        sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
        cal_type = sfpi_obj.get_calibration_type()
        sfpd_obj._calibration_type = cal_type

        offset = DOM_OFFSET
        transceiver_dom_info_dict = dict.fromkeys(transceiver_dom_info_dict_keys, 'N/A')

        dom_temperature_raw = self.__read_eeprom_specific_bytes((offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)

        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

        dom_voltage_raw = self.__read_eeprom_specific_bytes((offset + SFP_VOLT_OFFSET), SFP_VOLT_WIDTH)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

        dom_channel_monitor_raw = self.__read_eeprom_specific_bytes((offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
        if dom_channel_monitor_raw is not None:
            dom_voltage_data = sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
            transceiver_dom_info_dict['tx1power'] = dom_voltage_data['data']['TXPower']['value']
            transceiver_dom_info_dict['rx1power'] = dom_voltage_data['data']['RXPower']['value']
            transceiver_dom_info_dict['tx1bias'] = dom_voltage_data['data']['TXBias']['value']

        for key in transceiver_dom_info_dict:
            transceiver_dom_info_dict[key] = self.__convert_string_to_num(transceiver_dom_info_dict[key])

        transceiver_dom_info_dict['reset_status']       = self.get_reset_status()
        transceiver_dom_info_dict['rx_los']             = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault']           = self.get_tx_fault()
        transceiver_dom_info_dict['tx_disable']         = self.get_tx_disable()
        transceiver_dom_info_dict['tx_disable_channel'] = self.get_tx_disable_channel()
        transceiver_dom_info_dict['lp_mode']            = self.get_lpmode()

        return transceiver_dom_info_dict

    def get_transceiver_threshold_info(self):
        """
        Retrieves transceiver threshold info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        temphighalarm              |FLOAT          |High Alarm Threshold value of temperature in Celsius.
        templowalarm               |FLOAT          |Low Alarm Threshold value of temperature in Celsius.
        temphighwarning            |FLOAT          |High Warning Threshold value of temperature in Celsius.
        templowwarning             |FLOAT          |Low Warning Threshold value of temperature in Celsius.
        vcchighalarm               |FLOAT          |High Alarm Threshold value of supply voltage in mV.
        vcclowalarm                |FLOAT          |Low Alarm Threshold value of supply voltage in mV.
        vcchighwarning             |FLOAT          |High Warning Threshold value of supply voltage in mV.
        vcclowwarning              |FLOAT          |Low Warning Threshold value of supply voltage in mV.
        rxpowerhighalarm           |FLOAT          |High Alarm Threshold value of received power in dBm.
        rxpowerlowalarm            |FLOAT          |Low Alarm Threshold value of received power in dBm.
        rxpowerhighwarning         |FLOAT          |High Warning Threshold value of received power in dBm.
        rxpowerlowwarning          |FLOAT          |Low Warning Threshold value of received power in dBm.
        txpowerhighalarm           |FLOAT          |High Alarm Threshold value of transmit power in dBm.
        txpowerlowalarm            |FLOAT          |Low Alarm Threshold value of transmit power in dBm.
        txpowerhighwarning         |FLOAT          |High Warning Threshold value of transmit power in dBm.
        txpowerlowwarning          |FLOAT          |Low Warning Threshold value of transmit power in dBm.
        txbiashighalarm            |FLOAT          |High Alarm Threshold value of tx Bias Current in mA.
        txbiaslowalarm             |FLOAT          |Low Alarm Threshold value of tx Bias Current in mA.
        txbiashighwarning          |FLOAT          |High Warning Threshold value of tx Bias Current in mA.
        txbiaslowwarning           |FLOAT          |Low Warning Threshold value of tx Bias Current in mA.
        ========================================================================
        """
        transceiver_dom_threshold_info_dict_keys = ['temphighalarm',    'temphighwarning',    'templowalarm',    'templowwarning',
                                                    'vcchighalarm',     'vcchighwarning',     'vcclowalarm',     'vcclowwarning',
                                                    'rxpowerhighalarm', 'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                                                    'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
                                                    'txbiashighalarm',  'txbiashighwarning',  'txbiaslowalarm',  'txbiaslowwarning']

        sfpd_obj = sff8472Dom()

        if not self.get_presence() and not sfpd_obj:
            return {}

        sysfsfile_eeprom = None
        eeprom_raw = []
        bdata=''
        sysfsfile_eeprom = open(self.__eeprom_path, mode="rb", buffering=0)
        bdata = sysfsfile_eeprom.read()
        if len(bdata) <= 256:
            sysfsfile_eeprom.close()
            return None
        sysfsfile_eeprom.close()

        eeprom_ifraw = self.__read_eeprom_specific_bytes(0, DOM_OFFSET)
        sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
        cal_type = sfpi_obj.get_calibration_type()
        sfpd_obj._calibration_type = cal_type

        offset = DOM_OFFSET
        transceiver_dom_threshold_info_dict = dict.fromkeys(transceiver_dom_threshold_info_dict_keys, 'N/A')
        dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + SFP_MODULE_THRESHOLD_OFFSET), SFP_MODULE_THRESHOLD_WIDTH)
        if dom_module_threshold_raw is not None:
            dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(dom_module_threshold_raw, 0)

            transceiver_dom_threshold_info_dict['temphighalarm']      = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['templowalarm']       = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning']    = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowwarning']     = dom_module_threshold_data['data']['TempLowWarning']['value']

            transceiver_dom_threshold_info_dict['vcchighalarm']       = dom_module_threshold_data['data']['VoltageHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm']        = dom_module_threshold_data['data']['VoltageLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning']     = dom_module_threshold_data['data']['VoltageHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning']      = dom_module_threshold_data['data']['VoltageLowWarning']['value']

            transceiver_dom_threshold_info_dict['txbiashighalarm']    = dom_module_threshold_data['data']['BiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm']     = dom_module_threshold_data['data']['BiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning']  = dom_module_threshold_data['data']['BiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning']   = dom_module_threshold_data['data']['BiasLowWarning']['value']

            transceiver_dom_threshold_info_dict['txpowerhighalarm']   = dom_module_threshold_data['data']['TXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm']    = dom_module_threshold_data['data']['TXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning']  = dom_module_threshold_data['data']['TXPowerLowWarning']['value']

            transceiver_dom_threshold_info_dict['rxpowerhighalarm']   = dom_module_threshold_data['data']['RXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm']    = dom_module_threshold_data['data']['RXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning']  = dom_module_threshold_data['data']['RXPowerLowWarning']['value']

        for key in transceiver_dom_threshold_info_dict:
            transceiver_dom_threshold_info_dict[key] = self.__convert_string_to_num(transceiver_dom_threshold_info_dict[key])

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP

        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        rx_los = False
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw:
            data = int(status_control_raw[0], 16)
            rx_los = (sffbase().test_bit(data, 1) != 0)

        return rx_los

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP

        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        tx_fault = False
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw:
            data = int(status_control_raw[0], 16)
            tx_fault = (sffbase().test_bit(data, 2) != 0)

        return tx_fault

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        tx_disable = False
        tx_fault = False
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw:
            data = int(status_control_raw[0], 16)
            tx_disable_hard = (sffbase().test_bit(data, SFP_TX_DISABLE_HARD_BIT) != 0)
            tx_disable_soft = (sffbase().test_bit(data, SFP_TX_DISABLE_SOFT_BIT) != 0)
            tx_disable = tx_disable_hard | tx_disable_soft

        return tx_disable

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP

        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        # SFP doesn't support this feature
        return 0

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("temperature", "N/A")


    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            An integer number of supply voltage in mV
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        return transceiver_dom_info_dict.get("voltage", "N/A")

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP

        Returns:
            A list of four integer numbers, representing TX bias in mA
            for channel 0 to channel 4.
            Ex. ['110.09', '111.12', '108.21', '112.09']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_bs = transceiver_dom_info_dict.get("tx1bias", "N/A")
        return [tx1_bs, "N/A", "N/A", "N/A"] if transceiver_dom_info_dict else []

    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP

        Returns:
            A list of four integer numbers, representing received optical
            power in mW for channel 0 to channel 4.
            Ex. ['1.77', '1.71', '1.68', '1.70']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        rx1_pw = transceiver_dom_info_dict.get("rx1power", "N/A")
        return [rx1_pw, "N/A", "N/A", "N/A"] if transceiver_dom_info_dict else []

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP

        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        transceiver_dom_info_dict = self.get_transceiver_bulk_status()
        tx1_pw = transceiver_dom_info_dict.get("tx1power", "N/A")
        return [tx1_pw, "N/A", "N/A", "N/A"] if transceiver_dom_info_dict else []

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """
        # SFP doesn't support this feature
        return False

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        sysfs_eeprom_path = self.__eeprom_path
        status_control_raw = self.__read_eeprom_specific_bytes(SFP_STATUS_CONTROL_OFFSET, SFP_STATUS_CONTROL_WIDTH)
        if status_control_raw is not None:
            # Set bit 6 for Soft TX Disable Select
            # 01000000 = 64 and 10111111 = 191
            tx_disable_bit = 64 if tx_disable else 191
            status_control = int(status_control_raw[0], 16)
            tx_disable_ctl = (status_control | tx_disable_bit) if tx_disable else (status_control & tx_disable_bit)
            try:
                sysfsfile_eeprom = open(sysfs_eeprom_path, mode="r+b", buffering=0)
                buffer = create_string_buffer(1)
                buffer[0] = chr(tx_disable_ctl)
                # Write to eeprom
                sysfsfile_eeprom.seek(SFP_STATUS_CONTROL_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except:
                #print("Error: unable to open file: %s" % str(e))
                return False
            finally:
                if sysfsfile_eeprom:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True
        return False

    def tx_disable_channel(self, channel, disable):
        """
        Sets the tx_disable for specified SFP channels

        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable

        Returns:
            A boolean, True if successful, False if not
        """
        # SFP doesn't support this feature
        return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        # SFP doesn't support this feature
        return False

    def set_power_override(self, power_override, power_set):
        """
        Sets SFP power level using power_override and power_set

        Args:
            power_override :
                    A Boolean, True to override set_lpmode and use power_set
                    to control SFP power, False to disable SFP power control
                    through power_override/power_set and use set_lpmode
                    to control SFP power.
            power_set :
                    Only valid when power_override is True.
                    A Boolean, True to set SFP to low power mode, False to set
                    SFP to high power mode.

        Returns:
            A boolean, True if power-override and power_set are set successfully,
            False if not
        """
        # SFP doesn't support this feature
        return False
