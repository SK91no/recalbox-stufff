/**
 * @author Nicolas TESSIER aka Asthonishia
 */
export interface ControllersConfigResponse {
  'xarcade.enabled': {
    exist: boolean;
    value: boolean;
  };
  'bluetooth.ertm': {
    exist: boolean;
    value: boolean;
  };
  'gpio.enabled': {
    exist: boolean;
    value: boolean;
  };
  'ps3.driver': {
    exist: boolean;
    value: string;
  };
  'steam.enabled': {
    exist: boolean;
    value: boolean;
  };
  'gamecon.enabled': {
    exist: boolean;
    value: boolean;
  };
  'ps3.enabled': {
    exist: boolean;
    value: boolean;
  };
  'gpio.args': {
    exist: boolean;
    value: string;
  };
  'gamecon.args': {
    exist: boolean;
    value: string;
  };
  'bluetooth.enabled': {
    exist: boolean;
    value: boolean;
  };
  'db9.args': {
    exist: boolean;
    value: string;
  };
  'db9.enabled': {
    exist: boolean;
    value: boolean;
  };
  'joycond.enabled': {
    exist: boolean;
    value: boolean;
  };
  swapvalidateandcancel: {
    exist: boolean;
    value: boolean;
  };
  'bluetooth.autopaironboot': {
    exist: boolean;
    value: boolean;
  };
}

export interface ControllersConfigOptionsResponse {
  'xarcade.enabled': {
    type: string;
  };
  'bluetooth.ertm': {
    type: string;
  };
  'gpio.enabled': {
    type: string;
  };
  'ps3.driver': {
    type: string;
    allowedStringList: string[];
  };
  'steam.enabled': {
    type: string;
  };
  'gamecon.enabled': {
    type: string;
  };
  'ps3.enabled': {
    type: string;
  };
  'gpio.args': {
    type: string;
  };
  'gamecon.args': {
    type: string;
  };
  'bluetooth.enabled': {
    type: string;
  };
  'db9.args': {
    type: string;
  };
  'db9.enabled': {
    type: string;
  };
  'joycond.enable': {
    type: string;
  };
  swapvalidateandcancel: {
    type: string;
  };
  'bluetooth.autopaironboot': {
    type: string;
  };
}
