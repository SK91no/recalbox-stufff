/**
 * @author Pit64
 */
import { SimpleStringListObject } from 'stores/types/misc';

export interface HatConfigResponse {
  'wpaf.enabled': {
    exist: boolean;
    value: boolean;
  };
  'wpaf.board': {
    exist: boolean;
    value: string;
  };
}

export interface HatConfigOptionsResponse {
  'wpaf.enabled': {
    type: string;
  };
  'wpaf.board': SimpleStringListObject;
}
