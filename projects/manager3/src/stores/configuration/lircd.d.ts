/**
 * @author Pit64
 */

export interface LircdConfigResponse {
  enabled: {
    exist: boolean;
    value: boolean;
  };
}

export interface LircdConfigOptionsResponse {
  enabled: {
    type: string;
    value: boolean;
  };
}
