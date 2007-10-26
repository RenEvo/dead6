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
Imports RenEvo.Dead6.Tools.Interfaces
'Documentation: GenericPlugin

Public Class GenericPlugin
    Implements IPlugin

#Region " IPlugin "

    Public ReadOnly Property Author() As String Implements Interfaces.IPlugin.Author
        Get
            Return "Tom 'Dante' Anderson"
        End Get
    End Property

    Public ReadOnly Property Description() As String Implements Interfaces.IPlugin.Description
        Get
            Return "Just a generic plugin"
        End Get
    End Property

    Public ReadOnly Property Name() As String Implements Interfaces.IPlugin.Name
        Get
            Return "Generic"
        End Get
    End Property

    Public ReadOnly Property Version() As System.Version Implements Interfaces.IPlugin.Version
        Get
            Return Me.GetType.Assembly.GetName().Version
        End Get
    End Property

    Public ReadOnly Property IsUICapable() As Boolean Implements Interfaces.IPlugin.IsUICapable
        Get
            Return True
        End Get
    End Property

    Private m_Control As New System.Windows.Forms.WebBrowser()
    Public ReadOnly Property Control() As System.Windows.Forms.Control Implements Interfaces.IPlugin.Control
        Get
            m_Control.Navigate("http://dead6.renevo.com")
            Return m_Control
        End Get
    End Property

#End Region

#Region " IDisposable "

    Private disposedValue As Boolean = False        ' To detect redundant calls

    ' IDisposable
    Protected Overridable Sub Dispose(ByVal disposing As Boolean)
        If Not Me.disposedValue Then
            If disposing Then

            End If

        End If
        Me.disposedValue = True
    End Sub

    ' This code added by Visual Basic to correctly implement the disposable pattern.
    Public Sub Dispose() Implements IDisposable.Dispose
        ' Do not change this code.  Put cleanup code in Dispose(ByVal disposing As Boolean) above.
        Dispose(True)
        GC.SuppressFinalize(Me)
    End Sub

#End Region

End Class
