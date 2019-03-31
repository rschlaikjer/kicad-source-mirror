/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_track.h
 * @brief Definitions for tracks, vias and zones.
 */

#ifndef CLASS_TRACK_H
#define CLASS_TRACK_H


#include <pcbnew.h>
#include <class_board_item.h>
#include <board_connected_item.h>
#include <pcb_display_options.h>

#include <trigo.h>


class TRACK;
class VIA;
class D_PAD;
class MSG_PANEL_ITEM;
class SHAPE_POLY_SET;


// Flag used in locate routines (from which endpoint work)
enum ENDPOINT_T {
    ENDPOINT_START = 0,
    ENDPOINT_END = 1
};

// Via types
// Note that this enum must be synchronized to GAL_LAYER_ID
enum VIATYPE_T
{
    VIA_THROUGH      = 3,      /* Always a through hole via */
    VIA_BLIND_BURIED = 2,      /* this via can be on internal layers */
    VIA_MICROVIA     = 1,      /* this via which connect from an external layer
                                * to the near neighbor internal layer */
    VIA_NOT_DEFINED  = 0       /* not yet used */
};

#define UNDEFINED_DRILL_DIAMETER  -1       //< Undefined via drill diameter.

#define MIN_VIA_DRAW_SIZE          4       /// Minimum size in pixel for full drawing

// Used for tracks and vias for algorithmic safety, not to enforce constraints
#define GEOMETRY_MIN_SIZE ( int )( 0.001 * IU_PER_MM )


/**
 * Function GetTrack
 * is a helper function to locate a trace segment having an end point at \a aPosition
 * on \a aLayerMask starting at \a aStartTrace and end at \a aEndTrace.
 * <p>
 * The segments of track that are flagged as deleted or busy are ignored.  Layer
 * visibility is also ignored.
 * </p>
 * @param aStartTrace A pointer to the TRACK object to begin searching.
 * @param aEndTrace A pointer to the TRACK object to stop the search.  A NULL value
 *                  searches to the end of the list.
 * @param aPosition A wxPoint object containing the position to test.
 * @param aLayerMask A layer or layers to mask the hit test.  Use -1 to ignore
 *                   layer mask.
 * @return A TRACK object pointer if found otherwise NULL.
 */
TRACK* GetTrack( TRACK* aStartTrace, const TRACK* aEndTrace,
        const wxPoint& aPosition, LSET aLayerMask );

class TRACK : public BOARD_CONNECTED_ITEM
{
public:
    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_TRACE_T == aItem->Type();
    }

    BOARD_CONNECTED_ITEM* start;    // pointers to a connected item (pad or track)
    BOARD_CONNECTED_ITEM* end;

    double      m_Param;            // Auxiliary variable ( used in some computations )

    TRACK( BOARD_ITEM* aParent, KICAD_T idtype = PCB_TRACE_T );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    TRACK* Next() const { return static_cast<TRACK*>( Pnext ); }
    TRACK* Back() const { return static_cast<TRACK*>( Pback ); }

    virtual void Move( const wxPoint& aMoveVector ) override
    {
        m_Start += aMoveVector;
        m_End   += aMoveVector;
    }

    virtual void Rotate( const wxPoint& aRotCentre, double aAngle ) override;

    virtual void Flip( const wxPoint& aCentre ) override;

    void SetPosition( const wxPoint& aPos ) override { m_Start = aPos; }
    const wxPoint GetPosition() const override { return m_Start; }

    void SetWidth( int aWidth )                 { m_Width = aWidth; }
    int GetWidth() const                        { return m_Width; }

    void SetEnd( const wxPoint& aEnd )          { m_End = aEnd; }
    const wxPoint& GetEnd() const               { return m_End; }

    void SetStart( const wxPoint& aStart )      { m_Start = aStart; }
    const wxPoint& GetStart() const             { return m_Start; }


    /// Return the selected endpoint (start or end)
    const wxPoint& GetEndPoint( ENDPOINT_T aEndPoint ) const
    {
        if( aEndPoint == ENDPOINT_START )
            return m_Start;
        else
            return m_End;
    }

    // Virtual function
    const EDA_RECT GetBoundingBox() const override;

    bool IsLocked() const override
    {
        return GetState( TRACK_LOCKED );
    }

    void SetLocked( bool aLocked ) override
    {
        return SetState( TRACK_LOCKED, aLocked );
    }

    /**
     * Function GetBestInsertPoint
     * searches the "best" insertion point within the track linked list.
     * The best point is currently the end of the corresponding net code section.
     * (The BOARD::m_Track and BOARD::m_Zone lists are sorted by netcode.)
     * @param aPcb The BOARD to search for the insertion point.
     * @return TRACK* - the insertion point in the linked list.
     * this is the next item after the last item having my net code.
     * therefore the track to insert must be inserted before the insertion point.
     * if the best insertion point is the end of list, the returned value is NULL
     */
    TRACK* GetBestInsertPoint( BOARD* aPcb );

    /* Search (within the track linked list) the first segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK* GetStartNetCode( int NetCode );

    /* Search (within the track linked list) the last segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK* GetEndNetCode( int NetCode );

    /**
     * Function GetLength
     * returns the length of the track using the hypotenuse calculation.
     * @return double - the length of the track
     */
    double GetLength() const
    {
        return GetLineLength( m_Start, m_End );
    }

    /* Display on screen: */
    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset ) override;

    /**
     * Function TransformShapeWithClearanceToPolygon
     * Convert the track shape to a closed polygon
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approximated by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
     * @param ignoreLineWidth = used for edge cut items where the line width is only
     * for visualization
     */
    void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                               int             aClearanceValue,
                                               int             aCircleToSegmentsCount,
                                               double          aCorrectionFactor,
                                               bool            ignoreLineWidth = false ) const override;
    /**
     * Function IsPointOnEnds
     * returns STARTPOINT if point if near (dist = min_dist) start point, ENDPOINT if
     * point if near (dist = min_dist) end point,STARTPOINT|ENDPOINT if point if near
     * (dist = min_dist) both ends, or 0 if none of the above.
     * if min_dist < 0: min_dist = track_width/2
     */
    STATUS_FLAGS IsPointOnEnds( const wxPoint& point, int min_dist = 0 );

    /**
     * Function IsNull
     * returns true if segment length is zero.
     */
    bool IsNull();

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    virtual bool HitTest( const wxPoint& aPosition ) const override;

    virtual bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const override;

    /**
     * Function GetVia
     * finds the first VIA object at \a aPosition on \a aLayer starting at the trace.
     *
     * @param aPosition The wxPoint to HitTest() against.
     * @param aLayer The layer to match, pass -1 for a don't care.
     * @return A pointer to a VIA object if found, else NULL.
     */
    VIA* GetVia( const wxPoint& aPosition, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    /**
     * Function GetVia
     * finds the first VIA object at \a aPosition on \a aLayer starting at the trace
     * and ending at \a aEndTrace.
     *
     * @param aEndTrace Pointer to the last TRACK object to end search.
     * @param aPosition The wxPoint to HitTest() against.
     * @param aLayerMask The layers to match, pass -1 for a don't care.
     * @return A pointer to a VIA object if found, else NULL.
     */
    VIA* GetVia( TRACK* aEndTrace, const wxPoint& aPosition, LSET aLayerMask );

    /**
     * Function GetTrack
     * returns the trace segment connected to the segment at \a aEndPoint from \a
     * aStartTrace to \a aEndTrace.
     *
     * @param aStartTrace A pointer to the TRACK object to begin searching.
     * @param aEndTrace A pointer to the TRACK object to stop the search.  A NULL value
     *                  searches to the end of the list.
     * @param aEndPoint The start or end point of the segment to test against.
     * @param aSameNetOnly if true stop searching when the netcode changes
     * @param aSequential If true, forces a forward sequential search,
     * which is restartable; the default search can be faster but the
     * position of the returned track in the list is unpredictable
     * @return A TRACK object pointer if found otherwise NULL.
     */
    TRACK* GetTrack( TRACK* aStartTrace, TRACK* aEndTrace, ENDPOINT_T aEndPoint,
            bool aSameNetOnly, bool aSequential );

    /**
     * Function GetEndSegments
     * get the segments connected to the end point of the track.
     *  return 1 if OK, 0 when a track is a closed loop
     *  and the beginning and the end of the track in *StartTrack and *EndTrack
     *  Modify *StartTrack en *EndTrack  :
     *  (*StartTrack)->m_Start coordinate is the beginning of the track
     *  (*EndTrack)->m_End coordinate is the end of the track
     *  Segments connected must be consecutive in list
     */
    int GetEndSegments( int NbSegm, TRACK** StartTrack, TRACK** EndTrack );

    wxString GetClass() const override
    {
        return wxT( "TRACK" );
    }

    /**
     * Function GetClearance
     * returns the clearance in internal units.  If \a aItem is not NULL then the
     * returned clearance is the greater of this object's clearance and
     * aItem's clearance.  If \a aItem is NULL, then this objects clearance
     * is returned.
     * @param aItem is another BOARD_CONNECTED_ITEM or NULL
     * @return int - the clearance in internal units.
     */
    virtual int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const override;

    virtual wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    virtual EDA_ITEM* Clone() const override;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual unsigned int ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    const BOX2I ViewBBox() const override;

    virtual void SwapData( BOARD_ITEM* aImage ) override;

#if defined (DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }

    /**
     * Function ShowState
     * converts a set of state bits to a wxString
     * @param stateBits Is an OR-ed together set of bits like BUSY, EDIT, etc.
     */
    static wxString ShowState( int stateBits );

#endif

protected:
    /**
     * Function GetMsgPanelInfoBase
     * Display info about the track segment only, and does not calculate the full track length
     * @param aList A list of #MSG_PANEL_ITEM objects to add status information.
     */
    virtual void GetMsgPanelInfoBase( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList );


    /**
     * Helper function for the common panel info */
    void GetMsgPanelInfoBase_Common( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList );

    /**
     * Helper for drawing the short netname in tracks */
    void DrawShortNetname( EDA_DRAW_PANEL* panel, wxDC* aDC, GR_DRAWMODE aDrawMode,
            COLOR4D aBgColor );

    int         m_Width;            ///< Thickness of track, or via diameter
    wxPoint     m_Start;            ///< Line start point
    wxPoint     m_End;              ///< Line end point

private:
    // make SetNext() and SetBack() private so that they may not be called from anywhere.
    // list management is done on TRACKs using DLIST<TRACK> only.
    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }
};


class SEGZONE : public TRACK
{
public:
    SEGZONE( BOARD_ITEM* aParent );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    wxString GetClass() const override
    {
        return wxT( "ZONE" );
    }


    SEGZONE* Next() const { return static_cast<SEGZONE*>( Pnext ); }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset ) override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

protected:
    void GetMsgPanelInfoBase( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;
};


class VIA : public TRACK
{
public:
    VIA( BOARD_ITEM* aParent );

    static inline bool ClassOf( const EDA_ITEM *aItem )
    {
        return aItem && PCB_VIA_T == aItem->Type();
    }

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset ) override;

    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override;

    virtual LSET GetLayerSet() const override;

    /**
     * Function SetLayerPair
     * For a via m_Layer contains the top layer, the other layer is in
     * m_BottomLayer
     * @param aTopLayer = first layer connected by the via
     * @param aBottomLayer = last layer connected by the via
     */
    void SetLayerPair( PCB_LAYER_ID aTopLayer, PCB_LAYER_ID aBottomLayer );

    void SetBottomLayer( PCB_LAYER_ID aLayer );
    void SetTopLayer( PCB_LAYER_ID aLayer );

    /**
     * Function LayerPair
     * Return the 2 layers used by  the via (the via actually uses
     * all layers between these 2 layers)
     *  @param top_layer = pointer to the first layer (can be null)
     *  @param bottom_layer = pointer to the last layer (can be null)
     */
    void LayerPair( PCB_LAYER_ID* top_layer, PCB_LAYER_ID* bottom_layer ) const;

    PCB_LAYER_ID TopLayer() const;
    PCB_LAYER_ID BottomLayer() const;

    /**
     * Function SanitizeLayers
     * Check so that the layers are correct dependin on the type of via, and
     * so that the top actually is on top.
     */
    void SanitizeLayers();

    const wxPoint GetPosition() const override {  return m_Start; }
    void SetPosition( const wxPoint& aPoint ) override { m_Start = aPoint;  m_End = aPoint; }

    virtual bool HitTest( const wxPoint& aPosition ) const override;

    virtual bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const override;

    wxString GetClass() const override
    {
        return wxT( "VIA" );
    }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual unsigned int ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    virtual void Flip( const wxPoint& aCentre ) override;

#if defined (DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    VIATYPE_T GetViaType() const          { return m_ViaType; }
    void SetViaType( VIATYPE_T aViaType ) { m_ViaType = aViaType; }

    /**
     * Function SetDrill
     * sets the drill value for vias.
     * @param aDrill is the new drill diameter
    */
    void SetDrill( int aDrill )             { m_Drill = aDrill; }

    /**
     * Function GetDrill
     * returns the local drill setting for this VIA.  If you want the calculated value,
     * use GetDrillValue() instead.
     */
    int GetDrill() const                    { return m_Drill; }

    /**
     * Function GetDrillValue
     * "calculates" the drill value for vias (m-Drill if > 0, or default
     * drill value for the board.
     * @return real drill_value
    */
    int GetDrillValue() const;

    /**
     * Function SetDrillDefault
     * sets the drill value for vias to the default value #UNDEFINED_DRILL_DIAMETER.
    */
    void SetDrillDefault()      { m_Drill = UNDEFINED_DRILL_DIAMETER; }

    /**
     * Function IsDrillDefault
     * @return true if the drill value is default value (-1)
    */
    bool IsDrillDefault() const { return m_Drill <= 0; }

    virtual void SwapData( BOARD_ITEM* aImage ) override;

protected:
    void GetMsgPanelInfoBase( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

private:
    /// The bottom layer of the via (the top layer is in m_Layer)
    PCB_LAYER_ID  m_BottomLayer;

    VIATYPE_T m_ViaType;        // Type of via

    int       m_Drill;          // for vias: via drill (- 1 for default value)
};


/// Scan a track list for the first VIA o NULL if not found (or NULL passed)
inline VIA* GetFirstVia( TRACK* aTrk, const TRACK* aStopPoint = NULL )
{
    while( aTrk && (aTrk != aStopPoint) && (aTrk->Type() != PCB_VIA_T) )
        aTrk = aTrk->Next();

    // It could stop because of the stop point, not on a via
    if( aTrk && (aTrk->Type() == PCB_VIA_T) )
        return static_cast<VIA*>( aTrk );
    else
        return NULL;
}


/// Scan a track list for the first TRACK object. Returns  NULL if not found (or NULL passed)
inline TRACK* GetFirstTrack( TRACK* aTrk, const TRACK* aStopPoint = NULL )
{
    while( aTrk && ( aTrk != aStopPoint ) && ( aTrk->Type() != PCB_TRACE_T ) )
        aTrk = aTrk->Next();

    // It could stop because of the stop point, not on a via
    if( aTrk && ( aTrk->Type() == PCB_TRACE_T ) )
        return static_cast<TRACK*>( aTrk );
    else
        return NULL;
}

#endif // CLASS_TRACK_H
