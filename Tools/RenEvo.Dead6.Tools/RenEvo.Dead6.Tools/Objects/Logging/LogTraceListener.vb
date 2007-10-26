'************************************************************************************
'*  Copyright (C) 2007  RenEvo Software & Designs
'************************************************************************************
'*  This program is free software; you can redistribute it and/or
'*  modify it under the terms of the GNU General Public License
'*  as published by the Free Software Foundation; either version 2
'*  of the License, or (at your option) any later version.
'*  
'*  This program is distributed in the hope that it will be useful,
'*  but WITHOUT ANY WARRANTY; without even the implied warranty of
'*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'*  GNU General Public License for more details.
'*  
'*  You should have received a copy of the GNU General Public License
'*  along with this program; if not, write to the Free Software
'*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'************************************************************************************
'Documentation: Logging.LogTraceListener
Namespace Logging

    ''' <summary>
    ''' Private implementation of a log file
    ''' </summary>
    ''' <remarks></remarks>
    Public Class LogTraceListener
        Inherits TraceListener

        ''' <summary>
        ''' Used for thread safe operations
        ''' </summary>
        ''' <remarks></remarks>
        Private m_LogFileMutex As Threading.Mutex = Nothing

        ''' <summary>
        ''' LogFileName = &lt;User Directory&gt;\RenEvo Software &amp; Designs\The Dead Six Tools\&lt;Assembly File Version&gt;\&lt;AssemblyName&gt;.log
        ''' </summary>
        ''' <remarks></remarks>
        Private m_LogFileName As String = String.Format("{0}\RenEvo Software & Designs\The Dead Six Tools\{1}\{2}{3:00000}.log", Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "1.0.0.0", "RenEvo.Dead6.Tools", System.IO.Directory.GetFiles(String.Format("{0}\RenEvo Software & Designs\The Dead Six Tools\{1}\", Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "1.0.0.0"), "*.log").Length)

        Public Sub New(ByVal Data As String)
            Me.New()
            If Data.Length > 0 Then
                m_LogFileName = Data
                SetupLogFile()
            End If
        End Sub

        Public Sub New()
            m_LogFileMutex = New Threading.Mutex(False, New Guid().ToString())
            SetupLogFile()
        End Sub

        Private Sub SetupLogFile()
            m_LogFileMutex.WaitOne()
            System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(m_LogFileName))
            If System.IO.File.Exists(m_LogFileName) Then
                System.IO.File.Delete(m_LogFileName)
            End If
            m_LogFileMutex.ReleaseMutex()
        End Sub

        Public Overloads Overrides Sub Write(ByVal message As String)
            m_LogFileMutex.WaitOne()
            System.IO.File.AppendAllText(m_LogFileName, message)
            m_LogFileMutex.ReleaseMutex()
        End Sub

        Public Overloads Overrides Sub WriteLine(ByVal message As String)
            Me.Write(FormatMessage(message) + Environment.NewLine)
        End Sub

        Protected Overrides Sub Dispose(ByVal disposing As Boolean)
            m_LogFileMutex = Nothing
            MyBase.Dispose(disposing)
        End Sub

        Private Function FormatMessage(ByVal message As String) As String
            Return String.Format("[{1:00}.{2:00}.{0:0000} {3:00}:{4:00}:{5:00}] {6}", Now.Year, Now.Month, Now.Day, Now.Hour, Now.Minute, Now.Second, message)
        End Function

    End Class

End Namespace