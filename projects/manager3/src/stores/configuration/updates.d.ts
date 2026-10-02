/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { StringListObject } from 'stores/types/misc';

export interface UpdatesConfigResponse {
  enabled: {
    exist: boolean;
    value: boolean;
  };
  type: {
    exist: boolean;
    value: string;
  };
}

export interface UpdatesConfigOptionsResponse {
  enabled: {
    type: string;
  };
  type: StringListObject;
}
