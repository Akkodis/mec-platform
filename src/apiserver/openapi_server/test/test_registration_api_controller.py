# coding: utf-8

from __future__ import absolute_import

from flask import json
from six import BytesIO

from swagger_server.models.data_flow import DataFlow  # noqa: E501
from swagger_server.test import BaseTestCase


class TestRegistrationAPIController(BaseTestCase):
    """RegistrationAPIController integration test stubs"""

    def test_add_dataflow(self):
        """Test case for add_dataflow

        Register a Dataflow
        """
        body = DataFlow()
        response = self.client.open(
            '/dataflows',
            method='POST',
            data=json.dumps(body),
            content_type='application/json')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_delete_a_dataflow(self):
        """Test case for delete_a_dataflow

        Delete a registered a Dataflow
        """
        response = self.client.open(
            '/dataflows/{dataflowid}'.format(dataflowid='dataflowid_example'),
            method='DELETE')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_update_dataflow(self):
        """Test case for update_dataflow

        Update a registered a Dataflow
        """
        body = DataFlow()
        response = self.client.open(
            '/dataflows/{dataflowid}'.format(dataflowid='dataflowid_example'),
            method='PUT',
            data=json.dumps(body),
            content_type='application/json')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))


if __name__ == '__main__':
    import unittest
    unittest.main()
