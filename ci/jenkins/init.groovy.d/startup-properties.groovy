/**
 * \file    startup-properties.groovy
 * \brief   Groovy script to allow “HTML Publisher” to display Coverage Report
 *          correctly.
 *
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * To run Jenkins plug-in _HTML Publisher_ we introduce this
 * _startup-properties.groovy_ file to _init.groovy.d_ path of our Jenkins home
 * that allows publishing HTML documents. In doing this we are declaring we
 * trust the HTML documents we produce in our project.
 *
 * Also this Jenkins install is intended to be a local installation only to aid
 * in the development process rather than a wider server or cloud setup. If this
 * Jenkins configuration is ever considered for wider usage please consider this
 * topic.
 *
 * Note security notification on this topic:
 *      jenkins.io/doc/book/security/configuring-content-security-policy/
 * Risk must be assess per HTML publishing application
 */

System.setProperty("hudson.model.DirectoryBrowserSupport.CSP", "")
