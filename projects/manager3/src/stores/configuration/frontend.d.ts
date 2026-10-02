/**
 * @author Pit64
 */

export interface FrontendConfigResponse {
  enableeditfavorites: {
    exist: boolean;
    value: boolean;
  };
  listfastmovenabled: {
    exist: boolean;
    value: boolean;
  };
}

export interface FrontendConfigOptionsResponse {
  enableeditfavorites: {
    type: string;
  };
  listfastmovenabled: {
    type: string;
  };
}
