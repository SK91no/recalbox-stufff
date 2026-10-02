//
// Created by bkg2k on 12/06/25.
//
#pragma once

//! Game type
enum class GameType : char
{
  Normal,               //!< Is a normal game/folder
  // Non-Arcade Parent/Child relation
  ParentGameFolded,     //!< Normal game with a ▶ to display a folded state
  ParentGameUnfolded,   //!< Normal game with a ▼ to display an unfolded state
  ChildGame,            //!< Normal game indented to display a child from an unfolded parent
  // Arcade Parent/Clone relation
  ArcadeParentAlone,    //!< Arcade parent with no clone
  ArcadeParentFolded,   //!< Arcade parent game with an arcade machine icon and a ▶ to display a folded state
  ArcadeParentUnfolded, //!< Arcade parent game with an arcade machine icon and a ▼ to display an unfolded state
  ArcadeClone,          //!< Arcade clone game with a bartop icon and indented to display a child from an unfolded parent
  ArcadeOrphan,         //!< Arcade clone displayed like a parent w/o fold indicator
  ArcadeBios,           //!< Arcade bios with special icon
  ArcadeIllegal,        //!< Incompatible arcade game displayed with a red cross
};
