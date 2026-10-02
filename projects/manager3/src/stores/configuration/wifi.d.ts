/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { SimpleStringListObject } from 'stores/types/misc';

export interface WifiBaseConfigResponse {
  key: {
    exist: boolean;
    value: string;
  };
  ssid: {
    exist: boolean;
    value: string;
  };
}

export interface WifiBaseConfigOptionsResponse {
  key: {
    exist: boolean;
    value: string;
  };
  ssid: {
    exist: boolean;
    value: string;
  };
}

export interface WifiConfigResponse extends WifiBaseConfigResponse {
  enabled: {
    exist: boolean;
    value: boolean;
  };
  netmask: {
    exist: boolean;
    value: string;
  };
  ip: {
    exist: boolean;
    value: string;
  };
  gateway: {
    exist: boolean;
    value: string;
  };
  region: {
    exist: boolean;
    value: string;
  };
  priority: {
    exist: boolean;
    value: number;
  };
  nameservers: {
    exist: boolean;
    value: string;
  };
  connect: {
    exist: boolean;
    value: boolean;
  };
}

export interface WifiConfigOptionsResponse extends WifiBaseConfigOptionsResponse {
  enabled: {
    type: string;
  };
  netmask: {
    type: string;
    allowedChars: string;
  };
  ip: {
    type: string;
    allowedChars: string;
  };
  gateway: {
    type: string;
    allowedChars: string;
  };
  region: SimpleStringListObject;
  priority: {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  nameservers: {
    type: string;
    allowedChars: string;
  };
  connect: {
    type: string;
    value: boolean;
  };
}
