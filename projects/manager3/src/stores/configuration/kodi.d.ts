/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { StringListObject, SimpleStringListObject } from 'stores/types/misc';

export interface KodiConfigResponse {
  enabled: {
    exist: boolean;
    value: boolean;
  };
  xbutton: {
    exist: boolean;
    value: boolean;
  };
  'network.waittime': {
    exist: boolean;
    value: number;
  };
  'network.waithost': {
    exist: boolean;
    value: string;
  };
  atstartup: {
    exist: boolean;
    value: boolean;
  };
  videomode: {
    exist: boolean;
    value: string;
  };
  'network.waitmode': {
    exist: boolean;
    value: string;
  };
}

export interface KodiConfigOptionsResponse {
  enabled: {
    type: string;
  };
  xbutton: {
    type: string;
    value: number;
  };
  'network.waittime': {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  'network.waithost': {
    type: string;
    allowedChars: string;
  };
  atstartup: {
    type: string;
  };
  videomode: StringListObject;
  'network.waitmode': SimpleStringListObject;
}
