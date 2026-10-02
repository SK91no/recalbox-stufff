/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { StringListObject, SimpleStringListObject } from 'stores/types/misc';

export interface ScraperConfigResponse {
  'screenscraper.password': {
    exist: boolean;
    value: string;
  };
  'screenscraper.media': {
    exist: boolean;
    value: string;
  };
  extractregionfromfilename: {
    exist: boolean;
    value: string;
  };
  getnamefrom: {
    exist: boolean;
    value: string;
  };
  'screenscraper.language': {
    exist: boolean;
    value: string;
  };
  'screenscraper.region': {
    exist: boolean;
    value: string;
  };
  'screenscraper.thumbnail': {
    exist: boolean;
    value: string;
  };
  'screenscraper.video': {
    exist: boolean;
    value: string;
  };
  'screenscraper.maps': {
    exist: boolean;
    value: boolean;
  };
  'screenscraper.manual': {
    exist: boolean;
    value: boolean;
  };
  'screenscraper.user': {
    exist: boolean;
    value: string;
  };
  source: {
    exist: boolean;
    value: string;
  };
  auto: {
    exist: boolean;
    value: boolean;
  };
  'screenscraper.p2k': {
    exist: boolean;
    value: boolean;
  };
  'screenscraper.regionPriority': {
    exist: boolean;
    value: string;
  };
}

export interface ScraperConfigOptionsResponse {
  'screenscraper.password': {
    type: string;
  };
  'screenscraper.media': SimpleStringListObject;
  extractregionfromfilename: {
    type: string;
  };
  getnamefrom: SimpleStringListObject;
  'screenscraper.language': StringListObject;
  'screenscraper.region': StringListObject;
  'screenscraper.thumbnail': SimpleStringListObject;
  'screenscraper.video': SimpleStringListObject;
  'screenscraper.maps': {
    type: string;
  };
  'screenscraper.manual': {
    type: string;
  };
  'screenscraper.user': {
    type: string;
  };
  source: StringListObject;
  auto: {
    type: string;
  };
  'screenscraper.p2k': {
    type: string;
  };
  'screenscraper.regionPriority': SimpleStringListObject;
}
