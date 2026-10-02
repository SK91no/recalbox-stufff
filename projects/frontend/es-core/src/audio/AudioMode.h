//
// Created by gugue_u on 24/03/2021.
//

#pragma once

//! Audio mode
enum class AudioMode
{
  MusicsOnly,            //!< Music only (no video music)
  VideosSoundOnly,       //!< Video sound (no music)
  MusicsAndVideosSound,  //!< Both music & video playing at the same time
  MusicsXorVideosSound,  //!< Music or video Music pauses when a video plays
  None,                  //!< No music & no video sound
};