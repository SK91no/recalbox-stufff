/**
 * @author Pit64
 */
import { SimpleStringListObject } from 'stores/types/misc';

export interface TateConfigResponse {
  gamerotation: {
    exist: boolean;
    value: string;
  };
}

export interface TateConfigOptionsResponse {
  gamerotation: SimpleStringListObject;
}
