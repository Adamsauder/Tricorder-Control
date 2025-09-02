import React from 'react';
import { Typography, Container, Paper, Box } from '@mui/material';

const TestDashboard: React.FC = () => {
  return (
    <Container maxWidth="lg" sx={{ mt: 4, mb: 4 }}>
      <Paper sx={{ p: 2 }}>
        <Box>
          <Typography variant="h4" component="h1" gutterBottom>
            Test Dashboard - Grouped View
          </Typography>
          <Typography variant="body1">
            This is a test to confirm React is working properly.
          </Typography>
          <Typography variant="body2" sx={{ mt: 2 }}>
            If you can see this, the React app is rendering correctly.
          </Typography>
        </Box>
      </Paper>
    </Container>
  );
};

export default TestDashboard;
