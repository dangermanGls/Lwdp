/*! \file Ix_StringConvert.h
 *  \brief Define text format converter interface: Ix_StringConvert
 *  \author Zhang Yun Gui, X3 C++ PluginFramework
 *  \date   2011.06.30
 */
#ifndef X3_UTIL_ISTRINGCONVERT_H_
#define X3_UTIL_ISTRINGCONVERT_H_

#include "ClsID_TextUtil.h"

//! Text format converter interface.
/*!
    \interface Ix_StringConvert
    \ingroup _GROUP_UTILITY_
    \see x3::CLSID_TextUtil, StringConvert(), Ix_TextFileUtil, ConvStr.h
*/
class Ix_StringConvert : public Ix_Object
{
public:
    DEFINE_IID(Ix_StringConvert)

    //! ȥ���������˵Ŀհ��ַ�����ǿո�ȫ�ǿո񡢻��з���
    /*!
        \param[in,out] text ��ת�����ַ���
        \param[in] targets Ҫȥ���ַ����ַ����ϣ�ΪNULL��ȡΪĬ�ϵĿհ��ַ�
        \return �Ƿ�ı����ı�
    */
    virtual bool TrimSpace(tstring& text, const tchar_* targets = NULL) = 0;

    //! ȥ�����ֿ�ͷ�Ŀհ��ַ�����ǿո�ȫ�ǿո񡢻��з���
    /*!
        \param[in,out] text ��ת�����ַ���
        \param[in] targets Ҫȥ���ַ����ַ����ϣ�ΪNULL��ȡΪĬ�ϵĿհ��ַ�
        \return �Ƿ�ı����ı�
    */
    virtual bool TrimLeft(tstring& text, const tchar_* targets = NULL) = 0;

    //! ȥ������ĩβ�Ŀհ��ַ�����ǿո�ȫ�ǿո񡢻��з���
    /*!
        \param[in,out] text ��ת�����ַ���
        \param[in] targets Ҫȥ���ַ����ַ����ϣ�ΪNULL��ȡΪĬ�ϵĿհ��ַ�
        \return �Ƿ�ı����ı�
    */
    virtual bool TrimRight(tstring& text, const tchar_* targets = NULL) = 0;

    //! ȥ���Ƿ��ַ�
    /*! ȥ��ASCII��С��0x20���ַ����Ʊ�������з��ͻس������⣩
        \param[in,out] text �����ת�����ַ���
        \param[in] targets Ҫȥ���ַ����ַ����ϣ�ΪNULL��ȡΪС��0x20���ַ�
        \return �Ƿ�ı����ı�
    */
    virtual bool RemoveInvalidChars(tstring& text, const tchar_* targets = NULL) = 0;

    //! �滻�ı������ҵ��������Ӵ��滻Ϊ�µ�����
    /*!
        \param[in,out] text ���滻���ַ���
        \param[in] match ƥ������
        \param[in] newtext ��ƥ�䵽�����ݻ�Ϊ��������
        \return �Ƿ�ı����ı�
    */
    virtual bool ReplaceAll(tstring& text, 
        const tstring& match, 
        const tstring& newtext) = 0;

    //! ��һ���ı��е��ض��ַ��滻Ϊ��Ӧ�������ַ�
    /*! ��text�в���match�е��κ��ַ������ҵ����ַ��滻Ϊchars�ж�Ӧ��ŵ��ַ�
        \param[in,out] text ���滻���ַ���
        \param[in] match ƥ�����ݣ�ֻҪ�������κ�һ���ַ���ͬ����ƥ��ɹ�
        \param[in] chars Ҫ�滻�ɵ����ַ��ļ��ϣ��ַ��������match��ͬ
        \return �Ƿ�ı����ı�
    */
    virtual bool ReplaceChar(tstring& text, 
        const tstring& match, 
        const tstring& chars) = 0;

    //! ��ȫ���ַ�ת��Ϊ����ַ�
    /*!
        \param[in,out] text ��ת�����ַ���
        \param[in] punct �Ƿ���ͬ������Ҳһ��ת��
        \return �Ƿ�ı����ı�
    */
    virtual bool ToDBC(tstring& text, bool punct = false) = 0;

    //! UNICODE��ת��ΪANSI��
    /*!
        \param text Ҫת�������ݣ�ΪUNICODE������UTF16����
        \param codepage Ҫ�������ֱ����ANSI���봮��
            ���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return ANSI���봮
    */
    virtual tstring ToAnsi(const tstring& text, int32_ codepage = 0) = 0;

    //! ANSI��ת��ΪUNICODE��
    /*!
        \param text Ҫת�������ݣ�ΪANSI���������Ϊcodepage
        \param codepage textΪ���ֱ��룬���� CP_UTF8 ��ʾUTF-8��ʽ���룬Ĭ��ΪCP_ACP
        \return UNICODE������UTF16����
    */
    virtual tstring ToUnicode(const tstring& text, int32_ codepage = 0) = 0;
};

namespace x3 {

//! Get text format converter object.
/*!
    \ingroup _GROUP_UTILITY_
    \see Ix_TextFileUtil
*/
inline Cx_Interface<Ix_StringConvert> StringConvert()
{
    Cx_Interface<Ix_StringConvert> pIFUtility(x3::CLSID_TextUtil);
#ifdef ASSERT
    ASSERT(pIFUtility.IsNotNull());
#endif
    return pIFUtility;
}

} // x3

#endif // X3_UTIL_ISTRINGCONVERT_H_
