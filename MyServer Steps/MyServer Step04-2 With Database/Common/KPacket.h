#pragma once
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/shared_ptr.hpp>
#include <sstream>
#include "NetUtil.h"
#include "KGen.h"
#include "KPool.h"


#undef  _ENUM
#define _ENUM( id ) id,
enum EPacketId
{ 
    #include "Packet.inc" 
};


/// @see    https://stackoverflow.com/questions/8815164/c-wrapping-vectorchar-with-istream
template<typename CharT, typename TraitsT = std::char_traits<CharT> >
class vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT>
{
public:
                        vectorwrapbuf( IN std::vector<CharT>& vec )
                        {
                            setg( vec.data(), vec.data(), vec.data() + vec.size() );
                        }
};//class vectorwrapbuf


#pragma pack( push, 1 )

class KPacket;
typedef boost::shared_ptr<KPacket>  KPacketPtr;
class KPacket : public KPool<KPacket>
{
public:
                        KPacket();
                        ~KPacket();
    KPacket&            operator=( const KPacket& right );

    template<typename T>
    void                SetData( unsigned short usPacketId, const T& data );

    const wchar_t*      GetIdWstr() const { return GetIdWstr( m_usPacketId ); }
    static const wchar_t*   
                        GetIdWstr( const unsigned short usPacketId );

    template<typename Archive>
    void                serialize( Archive& ar, const unsigned int version )
                        {
                            ar & m_nSenderUid;
                            ar & m_usPacketId;
                            ar & m_buffer;
                        }//serialize()

public:
    LONGLONG            m_nSenderUid;
    unsigned short      m_usPacketId;
    std::vector<char>   m_buffer;

protected:
    static const wchar_t*   
                        ms_szPacketId[];
};//class KPacket

#pragma pack( pop )


template<typename T>
void KPacket::SetData( unsigned short usPacketId, const T& data_ )
{
    //m_nSenderUid = nSenderUid;
    m_usPacketId = usPacketId;

    std::stringstream   ss;
    boost::archive::text_oarchive oa{ ss };
    oa << data_;

    std::string& str = ss.str();
    m_buffer.reserve( str.size() );
    m_buffer.assign( str.begin(), str.end() );
}//KPacket::SetData()


template<typename T>
bool BufferToPacket( IN std::vector<char>& buffer, OUT T& data )
{
    if( buffer.empty() == true )
        return false;

    // alternative (slow) implementation. jintaeks on 2017-08-24_20-08
    //std::stringstream ss;
    //std::copy(buffer.begin(), buffer.end(), std::ostream_iterator<char>(ss));
    //boost::archive::text_iarchive ia{ ss };
    //ia >> data;

    vectorwrapbuf<char> databuf( buffer );
    std::istream is( &databuf );
    boost::archive::text_iarchive ia{ is };
    ia >> data;
    return true;
}//BufferToPacket()


template<typename T>
void BufferToPacket( IN std::stringstream& ss_, OUT T& packet_ )
{
    boost::archive::text_iarchive ia{ ss_ };
    ia >> packet_;
}//BufferToPacket()


template<typename T>
void PacketToBuffer( IN T& packet_, OUT std::vector<char>& buffer_ )
{
    std::stringstream   ss;
    boost::archive::text_oarchive oa{ ss };
    oa << packet_;

    // set [out] parameter
    std::string& str = ss.str();
    buffer_.reserve( str.size() );
    buffer_.assign( str.begin(), str.end() );
}//PacketToBuffer()


template<typename T>
void PacketToBuffer( IN T& packet_, OUT std::stringstream& ss_ )
{
    boost::archive::text_oarchive oa{ ss_ };
    oa << packet_;
}//PacketToBuffer()

#define DECLARE_PACKET( id )      struct K##id

#define CASE_PACKET(id, packetType_) \
    case id: \
    { \
        packetType_ kPacket; \
        if( BufferToPacket( IN pkPacket_->m_buffer, OUT kPacket ) == false ) \
        { \
            BEGIN_LOG( cerr, L"deserialze failed." L#id L" - " L#packetType_ ); \
        } \
        else On_##id( pkPacket_->m_nSenderUid, kPacket); \
    } \
    break

#define DECLARE_ON_PACKET(id, packet)    void On_##id( LONGLONG, packet& )
