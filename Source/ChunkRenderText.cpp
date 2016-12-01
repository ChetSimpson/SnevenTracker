/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <map>
#include <vector>
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Compiler.h"
#include "Chunk.h"
#include "ChunkRenderText.h"

/**
 * Text chunk render, these methods will always output single byte strings
 *
 */

static const int DEFAULT_LINE_BREAK = 20;

// String render functions
const stChunkRenderFunc CChunkRenderText::RENDER_FUNCTIONS[] = {
	{CHUNK_HEADER,			&CChunkRenderText::StoreHeaderChunk},
	{CHUNK_SEQUENCE,		&CChunkRenderText::StoreSequenceChunk},
	{CHUNK_INSTRUMENT_LIST,	&CChunkRenderText::StoreInstrumentListChunk},
	{CHUNK_INSTRUMENT,		&CChunkRenderText::StoreInstrumentChunk},
	// // //
	{CHUNK_SONG_LIST,		&CChunkRenderText::StoreSongListChunk},
	{CHUNK_SONG,			&CChunkRenderText::StoreSongChunk},
	{CHUNK_FRAME_LIST,		&CChunkRenderText::StoreFrameListChunk},
	{CHUNK_FRAME,			&CChunkRenderText::StoreFrameChunk},
	{CHUNK_PATTERN,			&CChunkRenderText::StorePatternChunk},
	// // //
};

CChunkRenderText::CChunkRenderText(CFile *pFile) : m_pFile(pFile)
{
}

void CChunkRenderText::StoreChunks(const std::vector<CChunk*> &Chunks)
{
	// Generate strings
	for (std::vector<CChunk*>::const_iterator it = Chunks.begin(); it != Chunks.end(); ++it) {
		for (int j = 0; j < sizeof(RENDER_FUNCTIONS) / sizeof(stChunkRenderFunc); ++j) {
			if ((*it)->GetType() == RENDER_FUNCTIONS[j].type)
				(this->*RENDER_FUNCTIONS[j].function)(*it, m_pFile);
		}
	}

	// Write strings to file
	WriteFileString(CStringA("; FamiTracker exported music data: "), m_pFile);
	WriteFileString(CFamiTrackerDoc::GetDoc()->GetTitle(), m_pFile);
	WriteFileString(CStringA("\n;\n\n"), m_pFile);

	// Module header
	DumpStrings(CStringA("; Module header\n"), CStringA("\n"), m_headerStrings, m_pFile);

	// Instrument list
	DumpStrings(CStringA("; Instrument pointer list\n"), CStringA("\n"), m_instrumentListStrings, m_pFile);
	DumpStrings(CStringA("; Instruments\n"), CStringA(""), m_instrumentStrings, m_pFile);

	// Sequences
	DumpStrings(CStringA("; Sequences\n"), CStringA("\n"), m_sequenceStrings, m_pFile);

	// // //

	// Songs
	DumpStrings(CStringA("; Song pointer list\n"), CStringA("\n"), m_songListStrings, m_pFile);
	DumpStrings(CStringA("; Song info\n"), CStringA("\n"), m_songStrings, m_pFile);

	// Song data
	DumpStrings(CStringA(";\n; Pattern and frame data for all songs below\n;\n\n"), CStringA(""), m_songDataStrings, m_pFile);

	// Actual DPCM samples are stored later
}

// // //

void CChunkRenderText::DumpStrings(const CStringA &preStr, const CStringA &postStr, CStringArray &stringArray, CFile *pFile) const
{
	WriteFileString(preStr, pFile);

	for (int i = 0; i < stringArray.GetCount(); ++i) {
		WriteFileString(stringArray[i], pFile);
	}

	WriteFileString(postStr, pFile);
}

void CChunkRenderText::StoreHeaderChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();
	int i = 0;

	str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
	str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
	str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
	str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
	str.AppendFormat("\t.byte %i ; flags\n", pChunk->GetData(i++));
	if (pChunk->IsDataReference(i))
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));	// FDS waves
	str.AppendFormat("\t.word %i ; NTSC speed\n", pChunk->GetData(i++));
	str.AppendFormat("\t.word %i ; PAL speed\n", pChunk->GetData(i++));
	if (i < pChunk->GetLength())
		str.AppendFormat("\t.word %i ; N163 channels\n", pChunk->GetData(i++));	// N163 channels

	m_headerStrings.Add(str);
}

void CChunkRenderText::StoreInstrumentListChunk(CChunk *pChunk, CFile *pFile)
{
	CString str;

	// Store instrument pointers
	str.Format(_T("%s:\n"), pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat(_T("\t.word %s\n"), pChunk->GetDataRefName(i));
	}

	m_instrumentListStrings.Add(str);
}

void CChunkRenderText::StoreInstrumentChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	str.Format("%s:\n\t.byte %i\n", pChunk->GetLabel(), pChunk->GetData(0));

	for (int i = 1; i < len; ++i) {
		if (pChunk->IsDataReference(i)) {
			str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i));
		}
		else {
			if (pChunk->GetDataSize(i) == 1) {
				str.AppendFormat("\t.byte $%02X\n", pChunk->GetData(i));
			}
			else {
				str.AppendFormat("\t.word $%04X\n", pChunk->GetData(i));
			}
		}
	}

	str.Append("\n");

	m_instrumentStrings.Add(str);
}

void CChunkRenderText::StoreSequenceChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	str.Format("%s:\n", pChunk->GetLabel());
	StoreByteString(pChunk, str, DEFAULT_LINE_BREAK);

	m_sequenceStrings.Add(str);
}

// // //

void CChunkRenderText::StoreSongListChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	str.Format("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i));
	}

	m_songListStrings.Add(str);
}

void CChunkRenderText::StoreSongChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	str.Format("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength();) {
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
		str.AppendFormat("\t.byte %i\t; frame count\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; pattern length\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; speed\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; tempo\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; initial bank\n", pChunk->GetData(i++));
	}

	str.Append("\n");

	m_songStrings.Add(str);
}

void CChunkRenderText::StoreFrameListChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	// Pointers to frames
	str.Format("; Bank %i\n", pChunk->GetBank());
	str.AppendFormat("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i));
	}

	m_songDataStrings.Add(str);
}

void CChunkRenderText::StoreFrameChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	// Frame list
	str.Format("%s:\n\t.word ", pChunk->GetLabel());

	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataReference(i))
			str.AppendFormat("%s%s", (j++ > 0) ? _T(", ") : _T(""), pChunk->GetDataRefName(i));
	}

	// Bank values
	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataBank(i)) {
			if (j == 0) {
				str.AppendFormat("\n\t.byte ", pChunk->GetLabel());
			}
			str.AppendFormat("%s$%02X", (j++ > 0) ? _T(", ") : _T(""), pChunk->GetData(i));
		}
	}

	str.Append("\n");

	m_songDataStrings.Add(str);
}

void CChunkRenderText::StorePatternChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	// Patterns
	str.Format("; Bank %i\n", pChunk->GetBank());
	str.AppendFormat("%s:\n", pChunk->GetLabel());

	const std::vector<char> &vec = pChunk->GetStringData(0);
	len = vec.size();

	StoreByteString(&vec.front(), vec.size(), str, DEFAULT_LINE_BREAK);
/*
	for (int i = 0; i < len; ++i) {
		str.AppendFormat("$%02X", (unsigned char)vec[i]);
		if ((i % 20 == 19) && (i < len - 1))
			str.Append("\n\t.byte ");
		else {
			if (i < len - 1)
				str.Append(", ");
		}
	}
*/
	str.Append("\n");

	m_songDataStrings.Add(str);
}

// // //

void CChunkRenderText::WriteFileString(const CStringA &str, CFile *pFile) const
{
	pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
}

void CChunkRenderText::StoreByteString(const char *pData, int Len, CStringA &str, int LineBreak) const
{	
	str.Append("\t.byte ");

	for (int i = 0; i < Len; ++i) {
		str.AppendFormat("$%02X", (unsigned char)pData[i]);

		if ((i % LineBreak == (LineBreak - 1)) && (i < Len - 1))
			str.Append("\n\t.byte ");
		else if (i < Len - 1)
			str.Append(", ");
	}

	str.Append("\n");
}

void CChunkRenderText::StoreByteString(const CChunk *pChunk, CStringA &str, int LineBreak) const
{
	int len = pChunk->GetLength();
	
	str.Append("\t.byte ");

	for (int i = 0; i < len; ++i) {
		str.AppendFormat("$%02X", pChunk->GetData(i));

		if ((i % LineBreak == (LineBreak - 1)) && (i < len - 1))
			str.Append("\n\t.byte ");
		else if (i < len - 1)
			str.Append(", ");
	}

	str.Append("\n");
}
