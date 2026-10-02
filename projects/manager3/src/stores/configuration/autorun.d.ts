/**
 * @author Pit64
 */

export interface AutorunConfigResponse {
  enabled: {
    exist: boolean;
    value: boolean;
  };
  uuid: {
    exist: boolean;
    value: string;
  };
  gamepath: {
    exist: boolean;
    value: string;
  };
}

export interface AutorunConfigOptionsResponse {
  enabled: {
    type: string;
  };
  uuid: {
    type: string;
  };
  gamepath: {
    type: string;
  };
}
