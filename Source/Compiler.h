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

#pragma once

// NSF file header
struct stNSFHeader {
	unsigned char	Ident[5];
	unsigned char	Version;
	unsigned char	TotalSongs;
	unsigned char	StartSong;
	unsigned short	LoadAddr;
	unsigned short	InitAddr;
	unsigned short	PlayAddr;
	unsigned char	SongName[32];
	unsigned char	ArtistName[32];
	unsigned char	Copyright[32];
	unsigned short	Speed_NTSC;
	unsigned char	BankValues[8];
	unsigned short	Speed_PAL;
	unsigned char	Flags;
	unsigned char	SoundChip;
	unsigned char	Reserved[4];
};

struct driver_t;
class CChunk;
enum chunk_type_t;

/*
 * Logger class
 */
class CCompilerLog
{
public:
	virtual ~CCompilerLog() {}
	virtual void WriteLog(LPCTSTR text) = 0;
	virtual void Clear() = 0;
};

/*
 * The compiler
 */
class CCompiler
{
public:
	CCompiler(CFamiTrackerDoc *pDoc, CCompilerLog *pLogger);
	~CCompiler();
	
	void	ExportNSF(LPCTSTR lpszFileName, int MachineType);
	void	ExportNES(LPCTSTR lpszFileName, bool EnablePAL);
	void	ExportBIN(LPCTSTR lpszBIN_File);		// // //
	void	ExportPRG(LPCTSTR lpszFileName, bool EnablePAL);
	void	ExportASM(LPCTSTR lpszFileName);

private:
	bool	OpenFile(LPCTSTR lpszFileName, CFile &file) const;

	void	CreateHeader(stNSFHeader *pHeader, int MachineType) const;
	void	SetDriverSongAddress(char *pDriver, unsigned short Address) const;
#if 0
	void	WriteChannelMap();
	void	WriteChannelTypes();
#endif

	void	PatchVibratoTable(char *pDriver) const;

	char*	LoadDriver(const driver_t *pDriver, unsigned short Origin) const;

	// Compiler
	bool	CompileData();
	void	ResolveLabels();
	bool	ResolveLabelsBankswitched();
	void	CollectLabels(CMap<CStringA, LPCSTR, int, int> &labelMap) const;
	bool	CollectLabelsBankswitched(CMap<CStringA, LPCSTR, int, int> &labelMap);
	void	AssignLabels(CMap<CStringA, LPCSTR, int, int> &labelMap);
	void	AddBankswitching();
	void	Cleanup();

	void	ScanSong();
	// // //
	bool	IsPatternAddressed(unsigned int Track, int Pattern, int Channel) const;
	bool	IsInstrumentInPattern(int index) const;

	void	CreateMainHeader();
	void	CreateSequenceList();
	void	CreateInstrumentList();
	// // //
	void	CreateFrameList(unsigned int Track);

	int		StoreSequence(CSequence *pSeq, CStringA &label);
	// // //
	void	StoreSongs();
	void	StorePatterns(unsigned int Track);

	// Bankswitching functions
	// // //
	void	UpdateFrameBanks();
	void	UpdateSongBanks();
	void	ClearSongBanks();
	void	EnableBankswitching();

	// // //

	// File writing
	void	WriteAssembly(CFile *pFile);
	void	WriteBinary(CFile *pFile);
	// // //

	// Object list functions
	CChunk	*CreateChunk(chunk_type_t Type, CStringA label);
	CChunk	*GetObjectByRef(CStringA label) const;
	int		CountData() const;

	// Debugging
	void	Print(LPCTSTR text, ...) const;
	void	ClearLog() const;

public:
	static const int PATTERN_CHUNK_INDEX;

	static const int PAGE_SIZE;
	static const int PAGE_START;
	static const int PAGE_BANKED;
	static const int PAGE_SAMPLES;

	static const int PATTERN_SWITCH_BANK;

	// // //

	static const bool LAST_BANK_FIXED;

	// Labels
	static const char LABEL_SONG_LIST[];
	static const char LABEL_INSTRUMENT_LIST[];
	// // //
	static const char LABEL_SEQ_2A03[];
	// // //
	static const char LABEL_INSTRUMENT[];
	static const char LABEL_SONG[];
	static const char LABEL_SONG_FRAMES[];
	static const char LABEL_SONG_FRAME[];
	static const char LABEL_PATTERN[];

	// Flags
	static const int FLAG_BANKSWITCHED;
	static const int FLAG_VIBRATO;

protected:
	static CCompiler *pCompiler;			// Points to an active CCompiler object

public:
	static CCompiler *GetCompiler();		// Get the active CCompiler object, NULL otherwise

	static unsigned int AdjustSampleAddress(unsigned int Address);

private:
	CFamiTrackerDoc *m_pDocument;

	// Object lists
	std::vector<CChunk*> m_vChunks;
	std::vector<CChunk*> m_vSequenceChunks;
	std::vector<CChunk*> m_vInstrumentChunks;
	std::vector<CChunk*> m_vSongChunks;	
	std::vector<CChunk*> m_vFrameChunks;
	std::vector<CChunk*> m_vPatternChunks;
	// // //

	// Special objects
	// // //
	CChunk			*m_pHeaderChunk;

	// // //

	// Flags
	bool			m_bBankSwitched;

	// Driver
	const driver_t	*m_pDriverData;
	unsigned int	m_iVibratoTableLocation;

	// Sequences and instruments
	unsigned int	m_iInstruments;
	unsigned int	m_iAssignedInstruments[MAX_INSTRUMENTS];
	bool			m_bSequencesUsed2A03[MAX_SEQUENCES][SEQ_COUNT];
	// // //

	// General
	unsigned int	m_iMusicDataSize;		// All music data
	unsigned int	m_iDriverSize;			// Size of selected music driver
	// // //

	unsigned int	m_iLoadAddress;			// NSF load address
	unsigned int	m_iInitAddress;			// NSF init address
	unsigned int	m_iDriverAddress;		// Music driver location

	unsigned int	m_iTrackFrameSize[MAX_TRACKS];	// Cached song frame sizes

	unsigned int	m_iHeaderFlagOffset;	// Offset to flag location in main header
	unsigned int	m_iSongBankReference;	// Offset to bank value in song header

	unsigned int	m_iDuplicatePatterns;	// Number of duplicated patterns removed

	std::vector<int> m_vChanOrder;			// Channel order list

	// NSF banks
	// // //
	unsigned int	m_iLastBank;			// Last bank in the NSF file

	unsigned int	m_iSamplePointerBank;
	unsigned int	m_iSamplePointerOffset;

	// // //

	// Optimization
	CMap<UINT, UINT, CChunk*, CChunk*> m_PatternMap;
	CMap<CStringA, LPCSTR, CStringA, LPCSTR> m_DuplicateMap;

	// Debugging
	CCompilerLog	*m_pLogger;

	// Diagnostics
	unsigned int	m_iHashCollisions;
};
