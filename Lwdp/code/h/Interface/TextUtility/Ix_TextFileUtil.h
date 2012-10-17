/*! \file Ix_TextFileUtil.h
 *  \brief Define text format file I/O operation interface: Ix_TextFileUtil
 *  \author Zhang Yun Gui, X3 C++ PluginFramework
 *  \date   2011.06.30
 */
#ifndef X3_UTIL_ITEXTFILEUTIL_H_
#define X3_UTIL_ITEXTFILEUTIL_H_

#include "ClsID_TextUtil.h"

//! Text format file I/O operation interface.
/*!
    \interface Ix_TextFileUtil
    \ingroup _GROUP_UTILITY_
    \see x3::CLSID_TextUtil, TextFileUtil(), Ix_StringConvert
*/
class Ix_TextFileUtil : public Ix_Object
{
public:
    DEFINE_IID(Ix_TextFileUtil)

    //! ��ȡһ���ı��ļ�
    /*!
        \param[out] content ��ȡ��������
        \param[in] filename �ļ�ȫ��
        \param[in] limitMB �ļ��������ޣ���λΪMB��������ض�
        \param[in] codepage ����ļ���ǰ��ANSI���룬��ָ�������ֱ��룬
            ���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return �Ƿ�ȡ��������
        \see GetLineCount, SaveTextFile
    */
    virtual bool ReadTextFile(tstring& content, 
        const tstring& filename, 
        ULONG limitMB = 16, int32_ codepage = 0) = 0;

    //! ��һ�����ַ������ݱ��浽�ļ���
    /*!
        \param content Ҫ���������
        \param filename �ļ�ȫ��
        \param utf16 �ļ������ʽ, true: UTF16, false: ANSI/ASCII
        \param codepage ���Ҫ����ΪANSI�����ļ�����ָ�������ֱ��룬
            ���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return �Ƿ񱣴�ɹ�
        \see ReadTextFile
    */
    virtual bool SaveTextFile(const tstring& content, 
        const tstring& filename, 
        bool utf16 = true, int32_ codepage = 0) = 0;

    //! ��һ��ANSI�ַ������ݱ��浽�ļ���
    /*!
        \param content Ҫ���������
        \param filename �ļ�ȫ��
        \param utf16 �ļ������ʽ, true: UTF16, false: ANSI/ASCII
        \param codepage ���Ҫ����ΪUNICODE�����ļ�(UTF16)����ָ������
            ��content�����ֱ��룬���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return �Ƿ񱣴�ɹ�
    */
    virtual bool SaveTextFile(const tstring& content, 
        const tstring& filename, 
        bool utf16 = true, int32_ codepage = 0) = 0;

    //! ���һ���ļ��Ƿ�ΪUNICODE�ļ�
    /*! ����ļ�ǰ�漸���ַ��Ƿ�ΪFFFE���� UTF-16 (little-endian)
        \param[in] filename �ļ�ȫ��
        \param[out] utf16 �ļ������ʽ, true: UTF16, false: ANSI/ASCII
        \return �Ƿ���ɹ�
    */
    virtual bool IsUTF16File(const tstring& filename, bool& utf16) = 0;

    //! ���һ���ļ��Ƿ�ΪUTF8��ANSI��ʽ�ļ�
    /*! ����ļ�ǰ�漸���ַ��Ƿ�ΪEFBBBF���� UTF-8
        \param[in] filename �ļ�ȫ��
        \param[out] utf8 �ļ������ʽ�Ƿ�ΪUTF8
        \return �Ƿ���ɹ�
    */
    virtual bool IsUTF8File(const tstring& filename, bool& utf8) = 0;

    //! �����UTF16�ļ���ת��Ϊָ�������ANSI�����ļ�
    /*!
        \param filename �ļ�ȫ��
        \param codepage Ҫ����Ϊ���ֱ����ANSI�����ļ���
            ���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return true if successful.
    */
    virtual bool UnicodeToAnsi(const tstring& filename, int32_ codepage = 0) = 0;

    //! �����ANSI�����ļ���ת��ΪUTF16�ļ�
    /*!
        \param filename �ļ�ȫ��
        \param codepage ����ļ���ǰ��ANSI���룬��ָ�������ֱ��룬
            ���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return true if successful.
    */
    virtual bool AnsiToUnicode(const tstring& filename, int32_ codepage = 0) = 0;

    //! �õ��ı����ݵ�����
    virtual long_ GetLineCount(const tstring& text) = 0;

    //! �õ��ı����ݵ�һ������
    /*!
        \param[in] text �ı�����
        \param[in] line �кţ�0��ʾ��һ��
        \param[out] nextLine �����һ������(����NULL)����ʼ��ַ��ΪNULL�����
        \return ���кŶ�Ӧ��һ������
        \see ReadTextFile
    */
    virtual tstring GetLine(const tstring& text, 
        long_ line, const tchar_** nextLine = NULL) = 0;

    //! �����Ƿ�Ϊ�հ������У��ɰ�ǿո�ȫ�ǿո񡢻��з���ɣ�
    virtual bool IsSpaceLine(const tstring& text) = 0;
};

namespace x3 {

//! Get text format file I/O operator object.
/*!
    \ingroup _GROUP_UTILITY_
    \see Ix_StringConvert
*/
inline Cx_Interface<Ix_TextFileUtil> TextFileUtil()
{
    Cx_Interface<Ix_TextFileUtil> pIFUtility(x3::CLSID_TextUtil);
#ifdef ASSERT
    ASSERT(pIFUtility.IsNotNull());
#endif
    return pIFUtility;
}

} // x3

#endif // X3_UTIL_ITEXTFILEUTIL_H_
