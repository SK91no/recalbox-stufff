/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { StringListObject, SimpleStringListObject } from 'stores/types/misc';

export interface AudioConfigResponse {
  bgmusic: {
    exist: boolean;
    value: boolean;
  };
  volume: {
    exist: boolean;
    value: number;
  };
  'music.volume': {
    exist: boolean;
    value: number;
  };
  device: {
    exist: boolean;
    value: string;
  };
  mode: {
    exist: boolean;
    value: string;
  };
}

export interface AudioConfigOptionsResponse {
  bgmusic: {
    type: string;
  };
  volume: {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  'music.volume': {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  device: StringListObject;
  mode: SimpleStringListObject;
}
