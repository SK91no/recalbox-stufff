/**
 * @author Pit64
 */
import { SimpleStringListObject } from 'stores/types/misc';

export interface PinballConfigResponse {
  'screen1': {
    exist: boolean;
    value: string;
  },
  'screen1.resolution': {
    exist: boolean;
    value: string;
  },
  'screen2': {
    exist: boolean;
    value: string;
  },
  'screen2.resolution': {
    exist: boolean;
    value: string;
  },
  'cabinet': {
    exist: boolean;
    value: boolean;
  };
}

export interface PinballConfigOptionsResponse {
  'screen1': {
   type: string;
  };
  'screen1.resolution': SimpleStringListObject;
  'screen2': {
    type: string;
  };
  'screen2.resolution': SimpleStringListObject;
  'cabinet': {
    type: boolean;
  };
}
